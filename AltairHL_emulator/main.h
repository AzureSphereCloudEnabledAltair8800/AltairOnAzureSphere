#pragma once

// Hardware definition
#include "hw/altair.h"

//#include "dx_config.h"
#include "../IntercoreContract/intercore_contract.h"
#include "altair_config.h"
#include "dx_exit_codes.h"
#include "dx_gpio.h"
#include "dx_i2c.h"
#include "dx_intercore.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_version.h"
#include "onboard_sensors.h"

// System Libraries
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/powermanagement.h>
#include <applibs/storage.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Altair app
#include "cpu_monitor.h"
#include "iotc_manager.h"
#include "utils.h"
#include "web_socket_server.h"

// Intel 8080 emulator
#include "88dcdd.h"
#include "altair_panel.h"
#include "intel8080.h"
#include "io_ports.h"
#include "memory.h"

#define ALTAIR_EMULATOR_VERSION "4.3.3"
#define Log_Debug(f_, ...) dx_Log_Debug((f_), ##__VA_ARGS__)
#define DX_LOGGING_ENABLED FALSE
#define BASIC_SAMPLES_DIRECTORY "BasicSamples"


// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:climatemonitor;1"

#define CORE_ENVIRONMENT_COMPONENT_ID "2e319eae-7be5-4a0c-ba47-9353aa6ca96a"
#define CORE_FILESYSTEM_COMPONENT_ID  "9b684af8-21b9-42aa-91e4-621d5428e497"
#define CORE_SD_CARD_COMPONENT_ID     "005180bc-402f-4cb3-a662-72937dbcde47"

#ifdef ALTAIR_FRONT_PANEL_CLICK
#include "front_panel_click.h"
#endif

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
#include "front_panel_retro_click.h"
#endif

#ifdef ALTAIR_FRONT_PANEL_KIT
#include "front_panel_kit.h"
#endif // ALTAIR_FRONT_PANEL_KIT

#ifdef ALTAIR_FRONT_PANEL_NONE
#include "front_panel_none.h"
#endif // ALTAIR_FRONT_PANEL_NONE


static DX_MESSAGE_PROPERTY *diag_msg_properties[] = {
	&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"},
	&(DX_MESSAGE_PROPERTY){.key = "type", .value = "diagnostics"},
	&(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES diag_content_properties = {
	.contentEncoding = "utf-8", .contentType = "application/json"};

intel8080_t cpu;
uint8_t memory[64 * 1024]; // Altair system memory.

ENVIRONMENT_TELEMETRY environment;
ALTAIR_CONFIG_T altair_config;
timer_t watchdogTimer;
volatile ALTAIR_COMMAND cmd_switches;
WS_INPUT_BLOCK_T ws_input_block;


bool azure_connected                           = false;
bool renderText                                = false;
char msgBuffer[MSG_BUFFER_BYTES]               = {0};
const struct itimerspec watchdogInterval       = {{60, 0}, {60, 0}};
int altair_spi_fd                              = -1;
static char Log_Debug_Time_buffer[64]          = {0};
static const char *AltairMsg                   = "\x1b[2J\r\nAzure Sphere - Altair 8800 Emulator ";
static int app_fd                              = -1;
volatile bool send_partial_msg                 = false;
volatile CPU_OPERATING_MODE cpu_operating_mode = CPU_STOPPED;
volatile uint16_t bus_switches                 = 0x00;

// basic app load helpers.
static volatile bool haveAppLoad       = false;
static volatile bool haveCtrlPending   = false;
static volatile char haveCtrlCharacter = 0x00;

static volatile bool haveTerminalInputMessage  = false;
static volatile bool haveTerminalOutputMessage = false;
static volatile int altairInputBufReadIndex    = 0;
static volatile int altairOutputBufReadIndex   = 0;
static volatile int terminalInputMessageLen    = 0;
static volatile int terminalOutputMessageLen   = 0;

static volatile char *input_data = NULL;

static void spin_wait(volatile bool *flag);
static bool load_application(const char *fileName);

static DX_DECLARE_DEVICE_TWIN_HANDLER(led_brightness_handler);
static DX_DECLARE_TIMER_HANDLER(connection_status_led_off_handler);
static DX_DECLARE_TIMER_HANDLER(connection_status_led_on_handler);
static DX_DECLARE_TIMER_HANDLER(heart_beat_handler);
static DX_DECLARE_TIMER_HANDLER(onboard_temperature_handler);
static DX_DECLARE_TIMER_HANDLER(onboard_temperature_pressure);
static DX_DECLARE_TIMER_HANDLER(panel_refresh_handler);
static DX_DECLARE_TIMER_HANDLER(read_panel_handler);
static DX_DECLARE_TIMER_HANDLER(report_memory_usage);
static DX_DECLARE_TIMER_HANDLER(update_environment_handler);
static DX_DECLARE_TIMER_HANDLER(WatchdogMonitorTimerHandler);
static void *altair_thread(void *arg);

const uint8_t reverse_lut[16] = {
	0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

ONBOARD_TELEMETRY onboard_telemetry;

INTERCORE_DISK_DATA_BLOCK_T intercore_disk_block;

DX_INTERCORE_BINDING intercore_filesystem_ctx = {.sockFd = -1,
	.nonblocking_io                                      = true,
	.rtAppComponentId                                    = CORE_FILESYSTEM_COMPONENT_ID,
	.interCoreCallback                                   = NULL,
	.intercore_recv_block                                = &intercore_disk_block,
	.intercore_recv_block_length                         = sizeof(intercore_disk_block)};

DX_INTERCORE_BINDING intercore_sd_card_ctx = {.sockFd = -1,
	.nonblocking_io                                   = false,
	.rtAppComponentId                                 = CORE_SD_CARD_COMPONENT_ID,
	.interCoreCallback                                = NULL,
	.intercore_recv_block                             = &intercore_disk_block,
	.intercore_recv_block_length                      = sizeof(intercore_disk_block)};


#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

CLICK_4X4_BUTTON_MODE click_4x4_key_mode = CONTROL_MODE;

as1115_t retro_click = {.interfaceId = ISU2,
	.handle                          = -1,
	.bitmap64                        = 0,
	.keymap                          = 0,
	.debouncePeriodMilliseconds      = 500};

#endif //  ALTAIR_FRONT_PANEL_RETRO_CLICK

#ifdef ALTAIR_FRONT_PANEL_KIT

// static DX_GPIO memoryCS = { .pin = MEMORY_CS, .direction = DX_OUTPUT, .initialState =
// GPIO_Value_High, .invertPin = false, .name = "memory CS" }; static DX_GPIO sdCS = { .pin = SD_CS,
// .direction = DX_OUTPUT, .initialState = GPIO_Value_High, .invertPin = false, .name = "SD_CS" };

DX_GPIO_BINDING switches_chip_select = {.pin = SWITCHES_CHIP_SELECT,
	.direction                               = DX_OUTPUT,
	.initialState                            = GPIO_Value_High,
	.invertPin                               = false,
	.name                                    = "switches CS"};

DX_GPIO_BINDING switches_load = {.pin = SWITCHES_LOAD,
	.direction                        = DX_OUTPUT,
	.initialState                     = GPIO_Value_High,
	.invertPin                        = false,
	.name                             = "switchs Load"};

DX_GPIO_BINDING led_store = {.pin = LED_STORE,
	.direction                    = DX_OUTPUT,
	.initialState                 = GPIO_Value_High,
	.invertPin                    = false,
	.name                         = "LED store"};

static DX_GPIO_BINDING led_master_reset = {.pin = LED_MASTER_RESET,
	.direction                                  = DX_OUTPUT,
	.initialState                               = GPIO_Value_High,
	.invertPin                                  = false,
	.name                                       = "LED master reset"};

static DX_GPIO_BINDING led_output_enable = {.pin = LED_OUTPUT_ENABLE,
	.direction                                   = DX_OUTPUT,
	.initialState                                = GPIO_Value_Low,
	.invertPin                                   = false,
	.name                                        = "LED output enable"}; // set OE initial state low

#endif // ALTAIR_FRONT_PANEL_KIT



// clang-format off
// Common Timers

DX_TIMER_BINDING tmr_copyx_request = {.name = "tmr_copyx_request", .handler = copyx_request_handler};
DX_TIMER_BINDING tmr_deferred_command = {.name = "tmr_deferred_command", .handler = deferred_command_handler};
DX_TIMER_BINDING tmr_deferred_input = {.name = "tmr_deferred_input", .handler = deferred_input_handler};
DX_TIMER_BINDING tmr_deferred_port_out_json = {.name = "tmr_deferred_port_out_json", .handler = port_out_json_handler};
DX_TIMER_BINDING tmr_deferred_port_out_weather = {.name = "tmr_deferred_port_out_weather", .handler = port_out_weather_handler};
DX_TIMER_BINDING tmr_partial_message = {.delay = &(struct timespec){1, 0}, .name = "tmr_partial_message", .handler = partial_message_handler};
DX_TIMER_BINDING tmr_port_timer_expired = {.name = "tmr_port_timer_expired", .handler = port_timer_expired_handler};
DX_TIMER_BINDING tmr_turn_off_notifications = {.name = "tmr_turn_off_notifications", .handler = turn_off_notifications_handler};

static DX_TIMER_BINDING tmr_connection_status_led_off = {.name = "tmr_connection_status_led_off", .handler = connection_status_led_off_handler};
static DX_TIMER_BINDING tmr_connection_status_led_on = {.delay = &(struct timespec){1, 0}, .name = "tmr_connection_status_led_on", .handler = connection_status_led_on_handler};
static DX_TIMER_BINDING tmr_heart_beat = {.repeat = &(struct timespec){60, 0}, .name = "tmr_heart_beat", .handler = heart_beat_handler};
static DX_TIMER_BINDING tmr_report_memory_usage = {.repeat = &(struct timespec){45, 0}, .name = "tmr_report_memory_usage", .handler = report_memory_usage};
static DX_TIMER_BINDING tmr_tick_count = {.repeat = &(struct timespec){1, 0}, .name = "tmr_tick_count", .handler = tick_count_handler};
static DX_TIMER_BINDING tmr_update_environment = {.delay = &(struct timespec){2, 0}, .name = "tmr_update_environment", .handler = update_environment_handler};
static DX_TIMER_BINDING tmr_watchdog_monitor = {.repeat = &(struct timespec){15, 0}, .name = "tmr_watchdog_monitor", .handler = WatchdogMonitorTimerHandler};


#if defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_KIT)
static DX_TIMER_BINDING tmr_read_panel = {.delay = &(struct timespec){10, 0}, .name = "tmr_read_panel", .handler = read_panel_handler};
static DX_TIMER_BINDING tmr_panel_refresh = {.delay = &(struct timespec){10, 0}, .name = "tmr_panel_refresh", .handler = panel_refresh_handler};
#else
static DX_TIMER_BINDING tmr_read_panel = {.name = "tmr_read_panel", .handler = read_panel_handler};
static DX_TIMER_BINDING tmr_panel_refresh = {.name = "tmr_panel_refresh", .handler = panel_refresh_handler};
#endif

#ifdef OEM_AVNET
static DX_TIMER_BINDING tmr_read_onboard_temperature = {.repeat = &(struct timespec){75, 0}, .name = "tmr_read_onboard_temperature", .handler = onboard_temperature_handler};
static DX_TIMER_BINDING tmr_read_onboard_pressure = {.repeat = &(struct timespec){90, 0}, .name = "tmr_read_onboard_pressure", .handler = onboard_temperature_pressure};
#else
static DX_TIMER_BINDING tmr_read_onboard_temperature = {.name = "tmr_read_onboard_temperature", .handler = onboard_temperature_handler };
static DX_TIMER_BINDING tmr_read_onboard_pressure = {.name = "tmr_read_onboard_pressure", .handler = onboard_temperature_pressure};
#endif


// Azure IoT Central Properties (Device Twins)

DX_DEVICE_TWIN_BINDING dt_air_quality_index = {.propertyName = "AirQualityIndexUS", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_carbon_monoxide = {.propertyName = "CarbonMonoxide", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_nitrogen_monoxide = {.propertyName = "NitrogenMonoxide", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_nitrogen_dioxide = {.propertyName = "NitrogenDioxide", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_ozone = {.propertyName = "Ozone", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_sulphur_dioxide = {.propertyName = "SulphurDioxide", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_ammonia = {.propertyName = "Ammonia", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_pm2_5 = {.propertyName = "PM2_5", .twinType = DX_DEVICE_TWIN_FLOAT};
DX_DEVICE_TWIN_BINDING dt_pm10 = {.propertyName = "PM10", .twinType = DX_DEVICE_TWIN_FLOAT};

DX_DEVICE_TWIN_BINDING dt_humidity = {.propertyName = "Humidity", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_pressure = {.propertyName = "Pressure", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_temperature = {.propertyName = "Temperature", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_weather = {.propertyName = "Weather", .twinType = DX_DEVICE_TWIN_STRING};
DX_DEVICE_TWIN_BINDING dt_wind_direction = {.propertyName = "WindDirection", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_wind_speed = {.propertyName = "WindSpeed", .twinType = DX_DEVICE_TWIN_FLOAT};

DX_DEVICE_TWIN_BINDING dt_location = {.propertyName = "Location", .twinType = DX_DEVICE_TWIN_JSON_OBJECT};
DX_DEVICE_TWIN_BINDING dt_country = {.propertyName = "Country", .twinType = DX_DEVICE_TWIN_STRING};
DX_DEVICE_TWIN_BINDING dt_city = {.propertyName = "City", .twinType = DX_DEVICE_TWIN_STRING};

DX_DEVICE_TWIN_BINDING dt_filesystem_reads = {.propertyName = "FilesystemReads", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_filesystem_ext_reads = {.propertyName = "FilesystemExtReads", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_filesystem_ext_writes = {.propertyName = "FilesystemExtWrites", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_new_sessions = {.propertyName = "NewSessions", .twinType = DX_DEVICE_TWIN_INT};

static DX_DEVICE_TWIN_BINDING dt_ledBrightness = {.propertyName = "LedBrightness", .twinType = DX_DEVICE_TWIN_INT, .handler = led_brightness_handler};
static DX_DEVICE_TWIN_BINDING dt_deviceStartTimeUtc = {.propertyName = "StartTimeUTC", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_heartbeatUtc = {.propertyName = "HeartbeatUTC", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_softwareVersion = {.propertyName = "SoftwareVersion", .twinType = DX_DEVICE_TWIN_STRING};

// GPIO
#ifdef SEEED_STUDIO_MDB
DX_GPIO_BINDING gpioRed = {.pin = LED_RED, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = false, .name = "red led"};
DX_GPIO_BINDING gpioGreen = {.pin = LED_GREEN, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = false, .name = "green led"};
DX_GPIO_BINDING gpioBlue = {.pin = LED_BLUE, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = false, .name = "blue led"};
#else
DX_GPIO_BINDING gpioRed = {.pin = LED_RED, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "red led"};
DX_GPIO_BINDING gpioGreen = {.pin = LED_GREEN, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "green led"};
DX_GPIO_BINDING gpioBlue = {.pin = LED_BLUE, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "blue led"};
#endif

//static DX_GPIO_BINDING buttonA = {.pin = BUTTON_A, .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW, .name = "buttonA"};
DX_GPIO_BINDING buttonB = {.pin = BUTTON_B, .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW, .name = "buttonB"};
static DX_GPIO_BINDING azure_connected_led = {
	.pin = AZURE_CONNECTED_LED, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low,
	.invertPin = true, .name = "azure_connected_led"};


// clang-format on

// Initialize Sets

#ifdef SEEED_STUDIO_MDB
static DX_GPIO_BINDING *gpioSet[] = {&azure_connected_led, &gpioRed, &gpioGreen, &gpioBlue
#else
static DX_GPIO_BINDING *gpioSet[] = {&buttonB, &azure_connected_led, &gpioRed, &gpioGreen, &gpioBlue
#endif

#ifdef ALTAIR_FRONT_PANEL_KIT
	,
	&switches_load, &switches_chip_select, &led_master_reset, &led_store, &led_output_enable
//&memoryCS, &sdCS
#endif // ALTAIR_FRONT_PANEL_KIT
};

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
DX_I2C_BINDING i2c_as1115_retro = {
	.interfaceId = ISU2, .speedInHz = I2C_BUS_SPEED_FAST_PLUS, .name = "i2c_as1115_retro"};
DX_I2C_BINDING i2c_onboard_sensors = {
	.interfaceId = ISU2, .speedInHz = I2C_BUS_SPEED_FAST_PLUS, .name = "i2c_onboard_sensors"};
static DX_I2C_BINDING *i2c_bindings[] = {&i2c_as1115_retro, &i2c_onboard_sensors};
#else

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

#ifdef OEM_AVNET
DX_I2C_BINDING i2c_onboard_sensors = {
	.interfaceId = ISU2, .speedInHz = I2C_BUS_SPEED_FAST_PLUS, .name = "i2c_onboard_sensors"};
static DX_I2C_BINDING *i2c_bindings[] = {&i2c_onboard_sensors};
#else
// just create a placeholder i2c for onboard sensors
DX_I2C_BINDING i2c_onboard_sensors;
static DX_I2C_BINDING *i2c_bindings[] = {};
#endif

static DX_TIMER_BINDING *timerSet[] = {
	&tmr_connection_status_led_off,
	&tmr_connection_status_led_on,
	&tmr_copyx_request,
	&tmr_deferred_command,
	&tmr_deferred_input,
	&tmr_deferred_port_out_json,
	&tmr_deferred_port_out_weather,
	&tmr_heart_beat,
	&tmr_panel_refresh,
	&tmr_partial_message,
	&tmr_port_timer_expired,
	&tmr_read_onboard_pressure,
	&tmr_read_onboard_temperature,
	&tmr_read_panel,
	&tmr_report_memory_usage,
	&tmr_tick_count,
	&tmr_turn_off_notifications,
	&tmr_update_environment,
	&tmr_watchdog_monitor,
};

static DX_DEVICE_TWIN_BINDING *deviceTwinBindingSet[] = {
	&dt_ledBrightness,
	&dt_deviceStartTimeUtc,
	&dt_heartbeatUtc,
	&dt_softwareVersion,

	&dt_air_quality_index,
	&dt_carbon_monoxide,
	&dt_nitrogen_monoxide,
	&dt_nitrogen_dioxide,
	&dt_ozone,
	&dt_sulphur_dioxide,
	&dt_ammonia,
	&dt_pm2_5,
	&dt_pm10,

	&dt_temperature,
	&dt_pressure,
	&dt_humidity,
	&dt_wind_speed,
	&dt_wind_direction,
	&dt_weather,

	&dt_location,
	&dt_country,
	&dt_city,

	&dt_filesystem_reads, 
	&dt_filesystem_ext_reads, 
	&dt_filesystem_ext_writes, 
	&dt_new_sessions,
};

DX_DIRECT_METHOD_BINDING *directMethodBindingSet[] = {};