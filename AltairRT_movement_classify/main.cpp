#include <cstdio>
#include <stdio.h>

#include "../IntercoreContract/intercore_contract.h"

#include "FreeRTOS.h"
#include "task.h"
#include "printf.h"
#include "mt3620.h"
#include <semphr.h>

#include "os_hal_gpio.h"
#include "os_hal_uart.h"
// #include "os_hal_i2c.h"
#include "os_hal_mbox.h"
#include "os_hal_mbox_shared_mem.h"

#include "lsm6dso_driver.h"
#include "lsm6dso_reg.h"

#include "ei_run_classifier.h"

void *__dso_handle = (void *)&__dso_handle;

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_ACCELEROMETER
#error "Incompatible model, this example is only compatible with accelerometer data"
#endif

/******************************************************************************/
/* Configurations */
/******************************************************************************/
/* UART */
static const UART_PORT uart_port_num = OS_HAL_UART_ISU0;

// #define I2C_MAX_LEN 64
#define APP_STACK_SIZE_BYTES 1024

// Edge Impulse
static volatile float x, y, z;
static float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};
static float inference_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};

// To prevent false positives we smoothen the results, with readings=10 and time_between_readings=200
// we look at 2 seconds of data + (length of window (e.g. also 2 seconds)) for the result

// We use N number of readings to smoothen the results over
#define SMOOTHEN_OVER_READINGS 10
// Time between readings in milliseconds
#define SMOOTHEN_TIME_BETWEEN_READINGS 200

// Intercore declarations

/* Maximum mailbox buffer len.
 *    Maximum message len: 1024B
 *                         1024 is the maximum value when HL_APP invoke send().
 *    Component UUID len : 16B
 *    Reserved data len  : 4B
 */
#define MBOX_BUFFER_LEN_MAX 1024 + 20

INTERCORE_PREDICTION_BLOCK_T intercore_block;

/* Bitmap for IRQ enable. bit_0 and bit_1 are used to communicate with HL_APP */
static const uint32_t mbox_irq_status = 0x3;

SemaphoreHandle_t blockDeqSema;
SemaphoreHandle_t blockFifoSema;
static const u32 pay_load_start_offset = 20; /* UUID 16B, Reserved 4B */

u8 mbox_buf[MBOX_BUFFER_LEN_MAX];
u32 allocated_buf_size = MBOX_BUFFER_LEN_MAX;
u32 mbox_buf_len;
BufferHeader *outbound, *inbound;
u32 shared_buf_size;
size_t payloadStart = 20; /* UUID 16B, Reserved 4B */

/// <summary>
///     When sending a message, this is the recipient HLApp's component ID.
///     When receiving a message, this is the sender HLApp's component ID.
/// </summary>
typedef struct
{
    /// <summary>4-byte little-endian word</summary>
    uint32_t data1;
    /// <summary>2-byte little-endian half</summary>
    uint16_t data2;
    /// <summary>2-byte little-endian half</summary>
    uint16_t data3;
    /// <summary>2 bytes (big-endian) followed by 6 bytes (big-endian)</summary>
    uint8_t data4[8];
    /// <summary> 8 bytes reserved </summary>
    uint32_t reserved_word;
} ComponentId;

static const ComponentId hlAppId = {.data1 = 0xac8d863a,
                                    .data2 = 0x4424,
                                    .data3 = 0x11eb,
                                    .data4 = {0xb3, 0x78, 0x02, 0x42, 0xac, 0x13, 0x00, 0x02},
                                    .reserved_word = 0};

/******************************************************************************/
/* Application Hooks */
/******************************************************************************/
/* Hook for "stack over flow". */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("%s: %s\n", __func__, pcTaskName);
}

/* Hook for "memory allocation failed". */
extern "C" void vApplicationMallocFailedHook(void)
{
    printf("%s\n", __func__);
}

/* Hook for "printf". */
extern "C" void _putchar(char character)
{
    mtk_os_hal_uart_put_char(uart_port_num, character);
    if (character == '\n')
        mtk_os_hal_uart_put_char(uart_port_num, '\r');
}

/******************************************************************************/
/* Functions */
/******************************************************************************/
/* Mailbox Fifo Interrupt handler.
 * Mailbox Fifo Interrupt is triggered when mailbox fifo been R/W.
 *     data->event.channel: Channel_0 for A7, Channel_1 for the other M4.
 *     data->event.ne_sts: FIFO Non-Empty.interrupt
 *     data->event.nf_sts: FIFO Non-Full interrupt
 *     data->event.rd_int: Read FIFO interrupt
 *     data->event.wr_int: Write FIFO interrupt
 */
void mbox_fifo_cb(struct mtk_os_hal_mbox_cb_data *data)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (data->event.channel == OS_HAL_MBOX_CH0)
    {
        /* A7 core write data to mailbox fifo. */
        if (data->event.wr_int)
        {
            xSemaphoreGiveFromISR(blockFifoSema, &higher_priority_task_woken);
            portYIELD_FROM_ISR(higher_priority_task_woken);
        }
    }
}

/* SW Interrupt handler.
 * SW interrupt is triggered when:
 *    1. A7 read/write the shared memory.
 *    2. The other M4 triggers SW interrupt.
 *     data->swint.swint_channel: Channel_0 for A7, Channel_1 for the other M4.
 *     Channel_0:
 *         data->swint.swint_sts bit_0: A7 read data from mailbox
 *         data->swint.swint_sts bit_1: A7 write data to mailbox
 *     Channel_1:
 *         data->swint.swint_sts bit_0: M4 sw interrupt
 */
void mbox_swint_cb(struct mtk_os_hal_mbox_cb_data *data)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (data->swint.channel == OS_HAL_MBOX_CH0)
    {
        if (data->swint.swint_sts & (1 << 1))
        {
            xSemaphoreGiveFromISR(blockDeqSema, &higher_priority_task_woken);
            portYIELD_FROM_ISR(higher_priority_task_woken);
        }
    }
}

static void send_intercore_msg(INTERCORE_PREDICTION_BLOCK_T *data, size_t length)
{
    uint32_t dataSize;
    memcpy((void *)mbox_buf, &hlAppId, sizeof(hlAppId)); // copy high level appid to first 20 bytes
    memcpy(mbox_buf + payloadStart, data, length);
    dataSize = payloadStart + length;
    EnqueueData(inbound, outbound, shared_buf_size, mbox_buf, dataSize);
}

void update_intercore(const char *prediction)
{
    static char previous_prediction[64] = {0};

    if (strcmp(prediction, "uncertain") != 0 && strcmp(prediction, previous_prediction) != 0)
    {
        strcpy(previous_prediction, prediction);

        intercore_block.cmd = IC_PREDICTION;
        strcpy(intercore_block.PREDICTION, prediction);

        send_intercore_msg(&intercore_block, sizeof(intercore_block));
    }
}

void MBOXTask_A(void *pParameters)
{
    struct mbox_fifo_event mask;
    int result;
    INTERCORE_ML_CLASSIFY_BLOCK_T *in_data;

    printf("MBOX_A Task Started\n");

    /* Open the MBOX channel of A7 <-> M4 */
    mtk_os_hal_mbox_open_channel(OS_HAL_MBOX_CH0);

    blockDeqSema = xSemaphoreCreateBinary();
    blockFifoSema = xSemaphoreCreateBinary();

    /* Register interrupt callback */
    mask.channel = OS_HAL_MBOX_CH0;
    mask.ne_sts = 0; /* FIFO Non-Empty interrupt */
    mask.nf_sts = 0; /* FIFO Non-Full interrupt */
    mask.rd_int = 0; /* Read FIFO interrupt */
    mask.wr_int = 1; /* Write FIFO interrupt */
    mtk_os_hal_mbox_fifo_register_cb(OS_HAL_MBOX_CH0, mbox_fifo_cb, &mask);
    mtk_os_hal_mbox_sw_int_register_cb(OS_HAL_MBOX_CH0, mbox_swint_cb, mbox_irq_status);

    /* Get mailbox shared buffer size, defined by Azure Sphere OS. */
    if (GetIntercoreBuffers(&outbound, &inbound, &shared_buf_size) == -1)
    {
        printf("GetIntercoreBuffers failed\n");
        return;
    }

    printf("shared buf size = %d\n", shared_buf_size);
    printf("allocated buf size = %d\n", allocated_buf_size);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Handle A7 <--> M4 Communication */
        /* Init buffer */
        mbox_buf_len = allocated_buf_size;
        memset(mbox_buf, 0, allocated_buf_size);

        /* Read from A7, dequeue from mailbox */
        result = DequeueData(outbound, inbound, shared_buf_size, mbox_buf, &mbox_buf_len);
        if (result == -1 || mbox_buf_len < pay_load_start_offset)
        {
            xSemaphoreTake(blockDeqSema, portMAX_DELAY);
            continue;
        }

        in_data = (INTERCORE_ML_CLASSIFY_BLOCK_T *)(mbox_buf + payloadStart);
        x = in_data->x;
        y = in_data->y;
        z = in_data->z;
    }
}

void inference_task(void *pParameters)
{
    // struct that smoothens out the readings over time, to avoid misclassification if a single frame
    // happened to overlap with one of the classes in our training set
    ei_classifier_smoothen_t smoothen;

    ei_classifier_smoothen_init(&smoothen, SMOOTHEN_OVER_READINGS, SMOOTHEN_OVER_READINGS * 0.7,
                                0.8 /* confidence */, 0.3 /* max anomaly score */);

    static bool first_reading = true;

    printf("Inference Task Started\n");

    // wait until we have a full frame of data
    vTaskDelay(pdMS_TO_TICKS((EI_CLASSIFIER_INTERVAL_MS * EI_CLASSIFIER_RAW_SAMPLE_COUNT)));

    while (1)
    {

        // copy into working buffer (other buffer is used by other task)
        memcpy(inference_buffer, buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE * sizeof(float));

        // Turn the raw buffer in a signal which we can the classify
        signal_t signal;
        int err = numpy::signal_from_buffer(inference_buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0)
        {
            ei_printf("Failed to create signal from buffer (%d)\n", err);
            return;
        }

        ei_impulse_result_t result = {0};

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
        if (res != 0)
        {
            printf("run_classifier returned: %d\n", res);
            return;
        }

        if (first_reading)
        {
            printf("Timing = (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)\n",
                   result.timing.dsp, result.timing.classification, result.timing.anomaly);
            first_reading = false;
        }

        const char *prediction = ei_classifier_smoothen_update(&smoothen, &result);

        // printf("%s", prediction);

        update_intercore(prediction);

        // printf(" [ ");

        // //  "normal", "rattle", "updown", "bearings"
        // for (size_t ix = 0; ix < smoothen.count_size; ix++)
        // {
        //     printf("%u", smoothen.count[ix]);
        //     if (ix != smoothen.count_size + 1)
        //     {
        //         printf(", ");
        //     }
        // }
        // printf("]\n");

        vTaskDelay(pdMS_TO_TICKS(SMOOTHEN_TIME_BETWEEN_READINGS));
    }

    ei_classifier_smoothen_free(&smoothen);
}

void data_task(void *pParameters)
{
    xTaskCreate(inference_task, "Inferencing Task", APP_STACK_SIZE_BYTES, NULL, 2, NULL);

    while (1)
    {
        // roll the buffer -3 points so we can overwrite the last one
        numpy::roll(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, -3);

        buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 3] = x / 100.0f;
        buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 2] = y / 100.0f;
        buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 1] = z / 100.0f;

        vTaskDelay(pdMS_TO_TICKS(EI_CLASSIFIER_INTERVAL_MS));
    }
}

extern "C" _Noreturn void RTCoreMain(void)
{
    /* Setup Vector Table */
    NVIC_SetupVectorTable();

    /* Init UART */
    mtk_os_hal_uart_ctlr_init(uart_port_num);

    printf("\nAzure Sphere Edge Impulse classifier and anomaly detection\n");

    /* Create MBOX Task */
    xTaskCreate(MBOXTask_A, "MBOX_A Task", APP_STACK_SIZE_BYTES / 4, NULL, 4, NULL);

    xTaskCreate(data_task, "I2C Task", APP_STACK_SIZE_BYTES / 8, NULL, 4, NULL);

    vTaskStartScheduler();
    for (;;)
        __asm__("wfi");
}
