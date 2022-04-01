#include "intercore.h"
#include "intercore_contract.h"

#include "os_hal_uart.h"
#include "nvic.h"

#include "uthash.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// CacheEntry is 176 bytes
// 800 items is stable
#define MAX_CACHE_ITEMS 800
#define SECTOR_LENGTH 137

uint8_t mbox_local_buf[MBOX_BUFFER_LEN_MAX];
BufferHeader* outbound, * inbound;
volatile u8  blockDeqSema;
volatile u8  blockFifoSema;

/* Bitmap for IRQ enable. bit_0 and bit_1 are used to communicate with HL_APP */
uint32_t mbox_irq_status = 0x3;
size_t payloadStart = 20; /* UUID 16B, Reserved 4B */

u32 mbox_shared_buf_size = 0;

static const uint8_t uart_port_num = OS_HAL_UART_ISU3;

/******************************************************************************/
/* Applicaiton Hooks */
/******************************************************************************/
/* Hook for "printf". */
void _putchar(char character) {
    mtk_os_hal_uart_put_char(uart_port_num, character);
    if (character == '\n')
        mtk_os_hal_uart_put_char(uart_port_num, '\r');
}

struct CacheEntry {
    int sector_number_key;
    uint8_t sector[SECTOR_LENGTH];
    UT_hash_handle hh;
};

struct CacheEntry* cache = NULL;

static struct CacheEntry* find_in_cache(int sector_number_key)
{
    struct CacheEntry* entry;

    HASH_FIND_INT(cache, &sector_number_key, entry);
    if (entry) {
        // remove it (so the subsequent add will throw it on the front of the list)
        HASH_DELETE(hh, cache, entry);
        HASH_ADD_INT(cache, sector_number_key, entry);
        return entry;
    }
    return NULL;
}

static void add_to_cache(int sector_number_key, uint8_t* sector)
{
    struct CacheEntry* entry, * tmp_entry;

    HASH_FIND_INT(cache, &sector_number_key, entry);  /* id already in the hash? */
    if (entry == NULL) {
        entry = malloc(sizeof(struct CacheEntry));
        entry->sector_number_key = sector_number_key;

        HASH_ADD_INT(cache, sector_number_key, entry);
    }

    memcpy(entry->sector, sector, SECTOR_LENGTH);

    if (HASH_COUNT(cache) >= MAX_CACHE_ITEMS) {
        HASH_ITER(hh, cache, entry, tmp_entry) {
            // prune the first entry (loop is based on insertion order so this deletes the oldest item)
            HASH_DELETE(hh, cache, entry);
            free(entry);
            break;
        }
    }
}

// static void delete_all(int delete_count) {
//     // printf("Delete all %d\n", delete_count);
//     struct CacheEntry* entry, * tmp_entry;
//     HASH_ITER(hh, cache, entry, tmp_entry) {
//         HASH_DELETE(hh, cache, entry);
//         free(entry);
//     }
// }

// static uint16_t calc_crc(uint8_t* sector) {
//     uint16_t crc = 0x0000;
//     for (int x = 0; x < SECTOR_LENGTH; x++)
//     {
//         crc += sector[x];
//         crc &= 0xffff;
//     }
//     return crc;
// }

static void send_intercore_msg(size_t length) {
    uint32_t dataSize;
    memcpy((void*)mbox_local_buf, &hlAppId, sizeof(hlAppId));	// copy high level appid to first 20 bytes
    dataSize = payloadStart + length;
    EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, dataSize);
}

static void process_inbound_message() {
    u32 mbox_local_buf_len;
    int result;
    uint16_t sector_number = 0;
    struct CacheEntry* entry;
    size_t send_length = 0;
    INTERCORE_DISK_DATA_BLOCK_T* in_data;
    INTERCORE_DISK_DATA_BLOCK_T* out_data;

    mbox_local_buf_len = MBOX_BUFFER_LEN_MAX;
    result = DequeueData(outbound, inbound, mbox_shared_buf_size, mbox_local_buf, &mbox_local_buf_len);

    if (result == 0 && mbox_local_buf_len > payloadStart) {

        in_data = (INTERCORE_DISK_DATA_BLOCK_T*)(mbox_local_buf + payloadStart);

        sector_number = in_data->sector_number;

        switch (in_data->disk_ic_msg_type) {

        case DISK_IC_READ:

            out_data = (INTERCORE_DISK_DATA_BLOCK_T*)(mbox_local_buf + payloadStart);

            entry = find_in_cache(sector_number);
            if (entry == NULL) {
                send_length = 4;  // 4 bytes = 1 x msg type, 2 x sector_number, 1 x cached
                memset(out_data, 0x00, send_length);
            }
            else {
                send_length = sizeof(INTERCORE_DISK_DATA_BLOCK_T);
                out_data->sector_number = entry->sector_number_key;
                out_data->cached = true;
                memcpy(out_data->sector, entry->sector, SECTOR_LENGTH);
            }
            send_intercore_msg(send_length);
            break;
        case DISK_IC_WRITE:
            add_to_cache(sector_number, in_data->sector);
            break;
        default:
            break;
        }
    }
}

_Noreturn void RTCoreMain(void) {

    /* Init Vector Table */
    NVIC_SetupVectorTable();

    initialise_intercore_comms();

    for (;;) {

        if (blockDeqSema > 0) {
            process_inbound_message();
        }
    }
}
