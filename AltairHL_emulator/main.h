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
#include "wifi_config.h"

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

const char ALTAIR_EMULATOR_VERSION[]   = "4.6.7";
const char DEFAULT_NETWORK_INTERFACE[] = "wlan0";
#define Log_Debug(f_, ...)      dx_Log_Debug((f_), ##__VA_ARGS__)
#define DX_LOGGING_ENABLED      FALSE
#define BASIC_SAMPLES_DIRECTORY "BasicSamples"

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:climatemonitor;1"

enum PANEL_MODE_T panel_mode = PANEL_BUS_MODE;

#define CORE_ENVIRONMENT_COMPONENT_ID "2e319eae-7be5-4a0c-ba47-9353aa6ca96a"
#define CORE_FILESYSTEM_COMPONENT_ID  "9b684af8-21b9-42aa-91e4-621d5428e497"
#define CORE_SD_CARD_COMPONENT_ID     "005180bc-402f-4cb3-a662-72937dbcde47"
#define CORE_ML_CLASSIFY_COMPONENT_ID "AF8B26DB-355E-405C-BBDE-3B851668EE23"

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
#include "front_panel_retro_click.h"
#endif

#ifdef ALTAIR_FRONT_PANEL_KIT
#include "front_panel_kit.h"
#endif // ALTAIR_FRONT_PANEL_KIT

#ifdef ALTAIR_FRONT_PANEL_NONE
#include "front_panel_none.h"
#endif // ALTAIR_FRONT_PANEL_NONE

#ifdef OEM_AVNET
#include "light_sensor.h"
#endif // OEM_AVNET

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
ALTAIR_COMMAND cmd_switches;
WS_INPUT_BLOCK_T ws_input_block;

CPU_OPERATING_MODE cpu_operating_mode    = CPU_STARTING;
bool azure_connected                     = false;
bool send_partial_msg                    = false;
char msgBuffer[MSG_BUFFER_BYTES]         = {0};
const struct itimerspec watchdogInterval = {{60, 0}, {60, 0}};
int altair_spi_fd                        = -1;
static bool altair_i8080_running         = false;
static bool network_connected            = false;
static bool stop_cpu                     = false;
static bool terminal_io_activity         = false;
static char Log_Debug_Time_buffer[64]    = {0};
static const char *AltairMsg             = "\x1b[2J\r\nAzure Sphere - Altair 8800 Emulator ";
static int app_fd                        = -1;
static int inactivity_period             = 0;
uint16_t bus_switches                    = 0x00;

// basic app load helpers.
static bool haveAppLoad            = false;
static char terminalInputCharacter = 0x00;

static bool terminal_read_pending     = false;
static bool haveTerminalInputMessage  = false;
static bool haveTerminalOutputMessage = false;
static int altairInputBufReadIndex    = 0;
static int altairOutputBufReadIndex   = 0;
static int terminalInputMessageLen    = 0;
static int terminalOutputMessageLen   = 0;

static char *input_data = NULL;

static bool load_application(const char *fileName);
void altair_sleep(void);
void altair_wake(void);
static void send_terminal_character(char character, bool wait);
static void spin_wait(bool *flag);
static void start_network_interface(void);

static DX_DECLARE_TIMER_HANDLER(connection_status_led_off_handler);
static DX_DECLARE_TIMER_HANDLER(connection_status_led_on_handler);
static DX_DECLARE_TIMER_HANDLER(heart_beat_handler);
static DX_DECLARE_TIMER_HANDLER(initialize_environment_handler);
static DX_DECLARE_TIMER_HANDLER(network_state_handler);
static DX_DECLARE_TIMER_HANDLER(panel_refresh_handler);
static DX_DECLARE_TIMER_HANDLER(read_buttons_handler);
static DX_DECLARE_TIMER_HANDLER(read_panel_handler);
static DX_DECLARE_TIMER_HANDLER(report_memory_usage);
static DX_DECLARE_TIMER_HANDLER(terminal_io_monitor_handler);
static DX_DECLARE_TIMER_HANDLER(update_environment_handler);
static DX_DECLARE_TIMER_HANDLER(WatchdogMonitorTimerHandler);
static void *altair_thread(void *arg);

const uint8_t reverse_lut[16] = {
	0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

INTERCORE_DISK_DATA_BLOCK_T intercore_disk_block;
INTERCORE_ML_CLASSIFY_BLOCK_T intercore_ml_classify_block;

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

DX_INTERCORE_BINDING intercore_ml_classify_ctx = {.sockFd = -1,
	.nonblocking_io                                       = true,
	.rtAppComponentId                                     = CORE_ML_CLASSIFY_COMPONENT_ID,
	.interCoreCallback                                    = intercore_classify_response_handler,
	.intercore_recv_block                                 = &intercore_ml_classify_block,
	.intercore_recv_block_length                          = sizeof(intercore_ml_classify_block)};

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

// CLICK_4X4_BUTTON_MODE click_4x4_key_mode = CONTROL_MODE;
//
// as1115_t retro_click = {.interfaceId = ISU2,
//	.handle                          = -1,
//	.bitmap64                        = 0,
//	.keymap                          = 0,
//	.debouncePeriodMilliseconds      = 500};

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

DX_TIMER_BINDING tmr_display_ip_address = {.handler = display_ip_address_handler};
DX_TIMER_BINDING tmr_i8080_wakeup = {.name = "tmr_i8080_wakeup", .handler = tmr_i8080_wakeup_handler};
DX_TIMER_BINDING tmr_partial_message = {.repeat = &(struct timespec){0, 250 * ONE_MS}, .name = "tmr_partial_message", .handler = partial_message_handler};
DX_TIMER_BINDING tmr_read_accelerometer = {.name = "tmr_read_accelerometer", .handler = read_accelerometer_handler};
DX_TIMER_BINDING tmr_terminal_io_monitor = {.repeat = &(struct timespec){60, 0}, .name = "tmr_terminal_io_monitor", .handler = terminal_io_monitor_handler};
DX_TIMER_BINDING tmr_timer_millisecond_expired = {.name = "tmr_timer_millisecond_expired", .handler = timer_millisecond_expired_handler};
DX_TIMER_BINDING tmr_timer_seconds_expired = {.name = "tmr_timer_seconds_expired", .handler = timer_seconds_expired_handler};
DX_TIMER_BINDING tmr_ws_ping_pong = {.repeat = &(struct timespec){10, 0}, .name = "tmr_partial_message", .handler = ws_ping_pong_handler};

static DX_TIMER_BINDING tmr_connection_status_led_off = {.name = "tmr_connection_status_led_off", .handler = connection_status_led_off_handler};
static DX_TIMER_BINDING tmr_connection_status_led_on = {.delay = &(struct timespec){1, 0}, .name = "tmr_connection_status_led_on", .handler = connection_status_led_on_handler};
static DX_TIMER_BINDING tmr_heart_beat = {.repeat = &(struct timespec){60, 0}, .name = "tmr_heart_beat", .handler = heart_beat_handler};
static DX_TIMER_BINDING tmr_network_state = {.repeat = &(struct timespec){20, 0}, .name = "tmr_network_state", .handler = network_state_handler};
static DX_TIMER_BINDING tmr_read_buttons = {.delay = &(struct timespec){0, 250 * ONE_MS}, .name = "tmr_read_buttons", .handler = read_buttons_handler};
static DX_TIMER_BINDING tmr_report_memory_usage = {.repeat = &(struct timespec){45, 0}, .name = "tmr_report_memory_usage", .handler = report_memory_usage};
static DX_TIMER_BINDING tmr_tick_count = {.repeat = &(struct timespec){1, 0}, .name = "tmr_tick_count", .handler = tick_count_handler};
static DX_TIMER_BINDING tmr_update_environment = {.name = "tmr_update_environment", .handler = update_environment_handler};
static DX_TIMER_BINDING tmr_initialize_environment = {.delay = &(struct timespec){8, 0}, .name = "tmr_update_environment", .handler = initialize_environment_handler};
static DX_TIMER_BINDING tmr_watchdog_monitor = {.repeat = &(struct timespec){15, 0}, .name = "tmr_watchdog_monitor", .handler = WatchdogMonitorTimerHandler};

DX_ASYNC_BINDING async_accelerometer_start = {.name = "async_accelerometer_start", .handler = async_accelerometer_start_handler};
DX_ASYNC_BINDING async_accelerometer_stop = {.name = "async_accelerometer_stop", .handler = async_accelerometer_stop_handler};
DX_ASYNC_BINDING async_copyx_request = {.name = "async_copyx_request", .handler = async_copyx_request_handler};
DX_ASYNC_BINDING async_expire_session = { .name = "async_expire_session", .handler = async_expire_session_handler};
DX_ASYNC_BINDING async_power_management_disable = {.name = "async_power_management_disable", .handler = async_power_management_disable_handler};
DX_ASYNC_BINDING async_power_management_enable = {.name = "async_power_management_enable", .handler = async_power_management_enable_handler};
DX_ASYNC_BINDING async_power_management_sleep = {.name = "async_power_management_sleep", .handler = async_power_management_sleep_handler};
DX_ASYNC_BINDING async_power_management_wake = {.name = "async_power_management_wake", .handler = async_power_management_wake_handler};
DX_ASYNC_BINDING async_publish_json = {.name = "async_publish_json", .handler = async_publish_json_handler};
DX_ASYNC_BINDING async_publish_weather = {.name = "async_publish_weather", .handler = async_publish_weather_handler};
DX_ASYNC_BINDING async_set_millisecond_timer = {.name = "async_set_millisecond_timer", .handler = async_set_timer_millisecond_handler};
DX_ASYNC_BINDING async_set_seconds_timer = {.name = "async_set_seconds_timer", .handler = async_set_timer_seconds_handler};

#if defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_KIT)
DX_TIMER_BINDING tmr_read_panel = {.delay = &(struct timespec){1, 0}, .name = "tmr_read_panel", .handler = read_panel_handler};
DX_TIMER_BINDING tmr_refresh_panel = {.delay = &(struct timespec){1, 0}, .name = "tmr_refresh_panel", .handler = panel_refresh_handler};
#else
static DX_TIMER_BINDING tmr_read_panel = {.name = "tmr_read_panel", .handler = read_panel_handler};
static DX_TIMER_BINDING tmr_refresh_panel = {.name = "tmr_refresh_panel", .handler = panel_refresh_handler};
#endif

// Azure IoT Central Properties (Device Twins)

// DX_DEVICE_TWIN_BINDING dt_air_quality_index = {.propertyName = "AirQualityIndexUS", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_carbon_monoxide = {.propertyName = "CarbonMonoxide", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_nitrogen_monoxide = {.propertyName = "NitrogenMonoxide", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_nitrogen_dioxide = {.propertyName = "NitrogenDioxide", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_ozone = {.propertyName = "Ozone", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_sulphur_dioxide = {.propertyName = "SulphurDioxide", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_ammonia = {.propertyName = "Ammonia", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_pm2_5 = {.propertyName = "PM2_5", .twinType = DX_DEVICE_TWIN_FLOAT};
// DX_DEVICE_TWIN_BINDING dt_pm10 = {.propertyName = "PM10", .twinType = DX_DEVICE_TWIN_FLOAT};

// DX_DEVICE_TWIN_BINDING dt_humidity = {.propertyName = "Humidity", .twinType = DX_DEVICE_TWIN_INT};
// DX_DEVICE_TWIN_BINDING dt_pressure = {.propertyName = "Pressure", .twinType = DX_DEVICE_TWIN_INT};
// DX_DEVICE_TWIN_BINDING dt_temperature = {.propertyName = "Temperature", .twinType = DX_DEVICE_TWIN_INT};
// DX_DEVICE_TWIN_BINDING dt_weather = {.propertyName = "Weather", .twinType = DX_DEVICE_TWIN_STRING};
// DX_DEVICE_TWIN_BINDING dt_wind_direction = {.propertyName = "WindDirection", .twinType = DX_DEVICE_TWIN_INT};
// DX_DEVICE_TWIN_BINDING dt_wind_speed = {.propertyName = "WindSpeed", .twinType = DX_DEVICE_TWIN_FLOAT};

DX_DEVICE_TWIN_BINDING dt_location = {.propertyName = "Location", .twinType = DX_DEVICE_TWIN_JSON_OBJECT};
DX_DEVICE_TWIN_BINDING dt_country = {.propertyName = "Country", .twinType = DX_DEVICE_TWIN_STRING};
DX_DEVICE_TWIN_BINDING dt_city = {.propertyName = "City", .twinType = DX_DEVICE_TWIN_STRING};

DX_DEVICE_TWIN_BINDING dt_new_sessions = {.propertyName = "NewSessions", .twinType = DX_DEVICE_TWIN_INT};

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

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
static DX_GPIO_BINDING buttonA = {.pin = BUTTON_A, .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW, .name = "buttonA"};
#else
static DX_GPIO_BINDING buttonA = {.direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW, .name = "buttonA"};
#endif

DX_GPIO_BINDING buttonB = {.pin = BUTTON_B, .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW, .name = "buttonB"};
static DX_GPIO_BINDING azure_connected_led = {
	.pin = AZURE_CONNECTED_LED, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low,
	.invertPin = true, .name = "azure_connected_led"};

// clang-format on

// Initialize Sets

#ifdef SEEED_STUDIO_MDB
static DX_GPIO_BINDING *gpioSet[] = {&azure_connected_led, &gpioRed, &gpioGreen, &gpioBlue
#else
static DX_GPIO_BINDING *gpioSet[] = {&buttonA, &buttonB, &azure_connected_led, &gpioRed, &gpioGreen, &gpioBlue
#endif

#ifdef ALTAIR_FRONT_PANEL_KIT
	,
	&switches_load, &switches_chip_select, &led_master_reset, &led_store, &led_output_enable
//&memoryCS, &sdCS
#endif // ALTAIR_FRONT_PANEL_KIT
};

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
CLICK_4X4_BUTTON_MODE click_4x4_key_mode = OPERATING_MODE;

as1115_t retro_click = {
	.interfaceId = ISU2, .handle = -1, .bitmap64 = 0, .keymap = 0, .debouncePeriodMilliseconds = 500};

DX_I2C_BINDING i2c_as1115_retro = {
	.interfaceId = ISU2, .speedInHz = I2C_BUS_SPEED_FAST_PLUS, .name = "i2c_as1115_retro"};

DX_I2C_BINDING i2c_onboard_sensors = {
	.interfaceId = ISU2, .speedInHz = I2C_BUS_SPEED_FAST_PLUS, .name = "i2c_onboard_sensors"};
// Note, retroclick keypad shares i2c_as1115_retro fd
static DX_I2C_BINDING *i2c_bindings[] = {&i2c_as1115_retro, &i2c_onboard_sensors};
#else
#ifdef OEM_AVNET
DX_I2C_BINDING i2c_onboard_sensors = {
	.interfaceId = ISU2, .speedInHz = I2C_BUS_SPEED_FAST_PLUS, .name = "i2c_onboard_sensors"};
static DX_I2C_BINDING *i2c_bindings[] = {&i2c_onboard_sensors};
#else
// just create a placeholder i2c for onboard sensors
DX_I2C_BINDING i2c_onboard_sensors;
static DX_I2C_BINDING *i2c_bindings[] = {};
#endif // OEM_AVNET
#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

static DX_ASYNC_BINDING *async_bindings[] = {
	&async_accelerometer_start,
	&async_accelerometer_stop,
	&async_copyx_request,
	&async_expire_session,
	&async_power_management_disable,
	&async_power_management_enable,
	&async_power_management_sleep,
	&async_power_management_wake,
	&async_publish_json,
	&async_publish_weather,
	&async_set_millisecond_timer,
	&async_set_seconds_timer,
};

static DX_TIMER_BINDING *timerSet[] = {
	&tmr_connection_status_led_off,
	&tmr_connection_status_led_on,
	&tmr_display_ip_address,
	&tmr_heart_beat,
	&tmr_i8080_wakeup,
	&tmr_initialize_environment,
	&tmr_network_state,
	&tmr_partial_message,
	&tmr_read_buttons,
	&tmr_read_panel,
	&tmr_refresh_panel,
	&tmr_report_memory_usage,
	&tmr_terminal_io_monitor,
	&tmr_tick_count,
	&tmr_timer_millisecond_expired,
	&tmr_timer_seconds_expired,
	&tmr_update_environment,
	&tmr_watchdog_monitor,
	&tmr_ws_ping_pong,
};

static DX_DEVICE_TWIN_BINDING *deviceTwinBindingSet[] = {
	&dt_deviceStartTimeUtc,
	&dt_heartbeatUtc,
	&dt_softwareVersion,

	// &dt_air_quality_index,
	// &dt_carbon_monoxide,
	// &dt_nitrogen_monoxide,
	// &dt_nitrogen_dioxide,
	// &dt_ozone,
	// &dt_sulphur_dioxide,
	// &dt_ammonia,
	// &dt_pm2_5,
	// &dt_pm10,

	// &dt_temperature,
	// &dt_pressure,
	// &dt_humidity,
	// &dt_wind_speed,
	// &dt_wind_direction,
	// &dt_weather,

	&dt_location,
	&dt_country,
	&dt_city,

	&dt_new_sessions,
};

DX_DIRECT_METHOD_BINDING *directMethodBindingSet[] = {};