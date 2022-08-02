/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

/// <summary>
/// This timer extends the app level lease watchdog timer
/// </summary>
static DX_TIMER_HANDLER(WatchdogMonitorTimerHandler)
{
    timer_settime(watchdogTimer, 0, &watchdogInterval, NULL);
}
DX_TIMER_HANDLER_END

/// <summary>
/// Monitor Altair Terminal Input and Output. If no activity for 10 minutes then disable WiFi, and sleep the
/// Altair i8080
/// </summary>
static DX_TIMER_HANDLER(terminal_io_monitor_handler)
{
    if (terminal_io_activity)
    {
        inactivity_period    = 0;
        terminal_io_activity = false;
    }
    else
    {
        inactivity_period++;
        if (inactivity_period > 9 && !stop_cpu)
        {
            altair_sleep();
            inactivity_period = 0;
        }
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Get geo location and environment for geo location from Open Weather Map
/// Requires Open Weather Map free api key
/// </summary>
static DX_TIMER_HANDLER(update_environment_handler)
{
    if (dx_isNetworkConnected(network_interface))
    {
        update_weather();
    }

    dx_timerOneShotSet(&tmr_update_environment, &(struct timespec){30 * 60, 0});
}
DX_TIMER_HANDLER_END

/// <summary>
/// Initialise cloud weather and geo cloud services
/// </summary>
static DX_TIMER_HANDLER(initialize_environment_handler)
{
    if (dx_isNetworkConnected(network_interface))
    {
        init_environment(&altair_config);
        update_geo_location(&environment);
        dx_timerOneShotSet(&tmr_update_environment, &(struct timespec){1, 0});
    }
    else
    {
        dx_timerOneShotSet(&tmr_initialize_environment, &(struct timespec){8, 0});
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Reports memory usage to IoT Central
/// </summary>
static DX_TIMER_HANDLER(report_memory_usage)
{
    if (azure_connected && dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 1, DX_JSON_INT, "memoryUsage",
                               Applications_GetPeakUserModeMemoryUsageInKB()))
    {
        dx_azurePublish(msgBuffer, strlen(msgBuffer), diag_msg_properties, NELEMS(diag_msg_properties),
            &diag_content_properties);
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Reports IoT Central Heartbeat UTC Date and Time
/// </summary>
static DX_TIMER_HANDLER(heart_beat_handler)
{
    if (azure_connected)
    {
        dx_deviceTwinReportValue(&dt_heartbeatUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer)));
        dx_deviceTwinReportValue(&dt_new_sessions, dt_new_sessions.propertyValue);
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Read panel input
/// </summary>
static DX_TIMER_HANDLER(read_panel_handler)
{
    read_altair_panel_switches(process_control_panel_commands);
    dx_timerOneShotSet(&tmr_read_panel, &(struct timespec){0, 250 * ONE_MS});
}
DX_TIMER_HANDLER_END

/// <summary>
/// Read buttons A Display IP address and B for sleep mode
/// </summary>
static DX_TIMER_HANDLER(read_buttons_handler)
{
    static GPIO_Value_Type buttonAState;
    static GPIO_Value_Type buttonBState;

    if (dx_gpioStateGet(&buttonA, &buttonAState))
    {
        getIP();
    }

    if (cpu_operating_mode != CPU_STOPPED)
    {
        if (dx_gpioStateGet(&buttonB, &buttonBState))
        {
            if (stop_cpu)
            {
                altair_wake();
            }
            else
            {
                altair_sleep();
                dx_timerOneShotSet(&tmr_read_buttons, &(struct timespec){2, 0});
                return;
            }
        }
    }
    dx_timerOneShotSet(&tmr_read_buttons, &(struct timespec){0, 250 * ONE_MS});
}
DX_TIMER_HANDLER_END

/// <summary>
/// Updates the PI Sense HAT with Altair address bus, databus, and CPU state
/// </summary>
static DX_TIMER_HANDLER(panel_refresh_handler)
{
    static uint8_t last_status = 0;
    static uint8_t last_data   = 0;
    static uint16_t last_bus   = 0;

    if (panel_mode == PANEL_BUS_MODE)
    {
        uint8_t status = cpu.cpuStatus;
        uint8_t data   = cpu.data_bus;
        uint16_t bus   = cpu.address_bus;

        if (status != last_status || data != last_data || bus != last_bus ||
            cpu_operating_mode == CPU_STARTING)
        {
            last_status = status;
            last_data   = data;
            last_bus    = bus;

            status = (uint8_t)(reverse_lut[(status & 0xf0) >> 4] | reverse_lut[status & 0xf] << 4);
            data   = (uint8_t)(reverse_lut[(data & 0xf0) >> 4] | reverse_lut[data & 0xf] << 4);
            bus = (uint16_t)(reverse_lut[(bus & 0xf000) >> 12] << 8 | reverse_lut[(bus & 0x0f00) >> 8] << 12 |
                             reverse_lut[(bus & 0xf0) >> 4] | reverse_lut[bus & 0xf] << 4);

            update_panel_status_leds(status, data, bus);
        }
    }

    dx_timerOneShotSet(&tmr_refresh_panel, &(struct timespec){0, 50 * ONE_MS});
}
DX_TIMER_HANDLER_END

/// <summary>
/// Network connection status LED blink off oneshot timer callback
/// </summary>
static DX_TIMER_HANDLER(connection_status_led_off_handler)
{
    dx_gpioOff(&azure_connected_led);
}
DX_TIMER_HANDLER_END

/// <summary>
/// Show network connection state
/// </summary>
static DX_TIMER_HANDLER(connection_status_led_on_handler)
{
    static int init_sequence = 25;

    if (stop_cpu)
    {
        dx_gpioOn(&azure_connected_led);
        // on for 100ms off for 8 seconds - 100ms
        dx_timerOneShotSet(&tmr_connection_status_led_on, &(struct timespec){8, 0});
        dx_timerOneShotSet(&tmr_connection_status_led_off, &(struct timespec){0, 100 * ONE_MS});
    }
    else if (init_sequence-- > 0)
    {
        dx_gpioOn(&azure_connected_led);
        // on for 100ms off for 100ms = 200 ms in total
        dx_timerOneShotSet(&tmr_connection_status_led_on, &(struct timespec){0, 200 * ONE_MS});
        dx_timerOneShotSet(&tmr_connection_status_led_off, &(struct timespec){0, 100 * ONE_MS});
    }
    else if (azure_connected)
    {
        dx_gpioOn(&azure_connected_led);
        // on for 100 off for 2700ms = 2800 ms in total
        dx_timerOneShotSet(&tmr_connection_status_led_on, &(struct timespec){2, 800 * ONE_MS});
        dx_timerOneShotSet(&tmr_connection_status_led_off, &(struct timespec){0, 100 * ONE_MS});
    }
    else if (network_connected)
    {
        dx_gpioOn(&azure_connected_led);
        // on for 700ms off for 1400ms = 1400 ms in total
        dx_timerOneShotSet(&tmr_connection_status_led_on, &(struct timespec){1, 400 * ONE_MS});
        dx_timerOneShotSet(&tmr_connection_status_led_off, &(struct timespec){0, 700 * ONE_MS});
    }
    else
    {
        dx_gpioOn(&azure_connected_led);
        // on for 1300ms off for 100ms = 1400 ms in total
        dx_timerOneShotSet(&tmr_connection_status_led_on, &(struct timespec){1, 400 * ONE_MS});
        dx_timerOneShotSet(&tmr_connection_status_led_off, &(struct timespec){0, 100 * ONE_MS});
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Display the device IP Address until first ws connection
/// </summary>
static DX_TIMER_HANDLER(show_ip_address_handler)
{
    getIP();
}
DX_TIMER_HANDLER_END

/// <summary>
/// Check network connection state every 20 seconds
/// </summary>
static DX_TIMER_HANDLER(network_state_handler)
{
    network_connected = dx_isNetworkReady();
}
DX_TIMER_HANDLER_END

/// <summary>
/// Clear sleep warning from display
/// </summary>
static DX_TIMER_HANDLER(sleep_warning_clear_handler)
{
#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
    as1115_panel_clear(&retro_click);
#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK
}
DX_TIMER_HANDLER_END

/// <summary>
/// Flash S for Sleep on the panel when the device is asleep
/// </summary>
static DX_TIMER_HANDLER(sleep_warning_handler)
{
#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
    gfx_load_character('S', retro_click.bitmap);

    gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
    gfx_reverse_panel(retro_click.bitmap);
    gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);

    as1115_panel_write(&retro_click);

    dx_timerOneShotSet(&tmr_sleep_warning_clear, &(struct timespec){0, 250 * ONE_MS});

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK
}
DX_TIMER_HANDLER_END

/// <summary>
/// Handler called to process inbound message
/// </summary>
void terminal_handler(WS_INPUT_BLOCK_T *in_block)
{
    char command[30];
    memset(command, 0x00, sizeof(command));

    pthread_mutex_lock(&in_block->block_lock);

    size_t application_message_size = in_block->length;
    char *data                      = in_block->buffer;

    // Was just enter pressed
    if (data[0] == '\r')
    {
        switch (cpu_operating_mode)
        {
            case CPU_RUNNING:
                send_terminal_character(0x0d, false);
                break;
            case CPU_STOPPED:
                data[0] = 0x00;
                process_virtual_input(data);
                break;
            default:
                break;
        }
        goto cleanup;
    }

    // Is it a ctrl character
    if (application_message_size == 1 && data[0] > 0 && data[0] < 29)
    {
        // ctrl-m is mapped to ascii 28 to get around ctrl-m being /r
        if (data[0] == 28)
        {
            cpu_operating_mode = cpu_operating_mode == CPU_RUNNING ? CPU_STOPPED : CPU_RUNNING;
            if (cpu_operating_mode == CPU_STOPPED)
            {
                bus_switches = cpu.address_bus;
                publish_message("\r\nCPU MONITOR> ", 15);
            }
            else
            {
                send_terminal_character(0x0d, false);
            }
        }
        else // pass through the ctrl character
        {
            send_terminal_character(data[0], false);
        }
        goto cleanup;
    }

    // The web terminal must be in single char mode as char is not a ctrl or enter char
    // so feed to Altair terminal read
    if (application_message_size == 1)
    {
        if (cpu_operating_mode == CPU_RUNNING)
        {
            altairOutputBufReadIndex  = 0;
            terminalOutputMessageLen  = 0;
            terminalInputCharacter    = data[0];
            haveTerminalOutputMessage = true;
            // send_terminal_character(data[0], false);
        }
        else
        {
            data[0] = (char)toupper(data[0]);
            data[1] = 0x00;
            process_virtual_input(data);
        }
        goto cleanup;
    }

    // Check for loadx command
    // upper case the first 30 chars for command processing
    for (int i = 0; i < sizeof(command) - 1 && i < application_message_size - 1; i++)
    { // -1 to allow for trailing null
        command[i] = (char)toupper(data[i]);
    }

    // if command is loadx then try looking for in baked in samples
    if (strncmp(command, "LOADX ", 6) == 0 && application_message_size > 6 &&
        (command[application_message_size - 2] == '"'))
    {
        command[application_message_size - 2] = 0x00; // replace the '"' with \0
        load_application(&command[7]);
        goto cleanup;
    }

    switch (cpu_operating_mode)
    {
        case CPU_RUNNING:

            if (application_message_size > 0)
            {
                input_data = data;

                altairInputBufReadIndex  = 0;
                altairOutputBufReadIndex = 0;
                terminalInputMessageLen  = (int)application_message_size;
                terminalOutputMessageLen = (int)application_message_size - 1;

                haveTerminalInputMessage = haveTerminalOutputMessage = true;

                spin_wait(&haveTerminalInputMessage);
            }
            break;
        case CPU_STOPPED:
            process_virtual_input(command);
            break;
        default:
            break;
    }

cleanup:
    in_block->length = 0;
    pthread_mutex_unlock(&in_block->block_lock);
}

/// <summary>
/// Wake the Altair, enable WiFi, start timers and i8080 cpu emulator
/// </summary>
void altair_wake(void)
{
    if (altair_i8080_running)
    {
        return;
    }

    inactivity_period = 0;
    stop_cpu          = false;

    dx_timerStart(&tmr_read_panel);
    dx_timerStart(&tmr_refresh_panel);
    dx_timerStart(&tmr_terminal_io_monitor);

    dx_timerStop(&tmr_sleep_warning);

    PowerManagement_SetSystemPowerProfile(PowerManagement_HighPerformance);
    start_network_interface();
    dx_timerStart(&tmr_partial_message);

    dx_startThreadDetached(altair_thread, NULL, "altair_thread");
    while (!altair_i8080_running) // spin until i8080 thread starts
    {
        nanosleep(&(struct timespec){0, 1 * ONE_MS}, NULL);
    }
}

/// <summary>
/// Sleep the Altair, disable WiFi, stop timers and  stop the i8080 cpu emulator
/// </summary>
void altair_sleep(void)
{
    if (!altair_i8080_running)
    {
        return;
    }

    stop_cpu = true;

    while (altair_i8080_running) // spin until i8080 thread stops
    {
        nanosleep(&(struct timespec){0, 1 * ONE_MS}, NULL);
    }

    dx_timerStop(&tmr_read_panel);
    dx_timerStop(&tmr_refresh_panel);
    dx_timerStop(&tmr_terminal_io_monitor);
    dx_timerStop(&tmr_show_ip_address);

    dx_timerStart(&tmr_sleep_warning);

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
    as1115_clear(&retro_click);
#endif

    PowerManagement_SetSystemPowerProfile(PowerManagement_PowerSaver);
    Networking_SetInterfaceState(network_interface, false);

    dx_timerStop(&tmr_partial_message);
}

/// <summary>
/// Set up watchdog timer - the lease is extended via the WatchdogMonitorTimerHandler function
/// </summary>
/// <param name=""></param>
void SetupWatchdog(void)
{
    struct sigevent alarmEvent;
    alarmEvent.sigev_notify          = SIGEV_SIGNAL;
    alarmEvent.sigev_signo           = SIGALRM;
    alarmEvent.sigev_value.sival_ptr = &watchdogTimer;

    if (timer_create(CLOCK_MONOTONIC, &alarmEvent, &watchdogTimer) == 0)
    {
        if (timer_settime(watchdogTimer, 0, &watchdogInterval, NULL) == -1)
        {
            Log_Debug("Issue setting watchdog timer. %s %d\n", strerror(errno), errno);
        }
    }
}

/// <summary>
/// Send character by character to Altair terminal input when ready
/// </summary>
static void send_terminal_character(char character, bool wait)
{
    int retry              = 0;
    terminalInputCharacter = character;

    if (!wait)
    {
        return;
    }

    while (terminalInputCharacter && retry++ < 10)
    {
        nanosleep(&(struct timespec){0, 1 * ONE_MS}, NULL);
    }
}

/// <summary>
/// Sets wait for terminal io cmd to be processed and flag reset
/// </summary>
static void spin_wait(bool *flag)
{
    struct timespec delay = {0, 1 * ONE_MS};
    int retry             = 0;
    *flag                 = true;

    // wait max 200ms = 10 x 20ms
    while (*flag && retry++ < 10)
    {
        while (nanosleep(&delay, &delay))
            ;
    }
}

/// <summary>
/// Client connected successfully
/// </summary>
static void client_connected_cb(void)
{
    dx_timerStop(&tmr_show_ip_address);
    print_console_banner();
    cpu_operating_mode = CPU_RUNNING;
}

/// <summary>
/// Load sample BASIC applications
/// </summary>
/// <param name="fileName"></param>
/// <returns></returns>
static bool load_application(const char *fileName)
{
    int retry = 0;
    char filePathAndName[50];
    snprintf(filePathAndName, sizeof(filePathAndName), "%s/%s", BASIC_SAMPLES_DIRECTORY, fileName);

    // precaution
    if (app_fd != -1)
    {
        close(app_fd);
    }

    if ((app_fd = Storage_OpenFileInImagePackage(filePathAndName)) != -1)
    {
        send_terminal_character(0x0d, true);

        terminal_read_pending    = true;
        haveTerminalInputMessage = false;
        haveAppLoad              = true;

        retry = 0;
        while (haveAppLoad && retry++ < 20)
        {
            nanosleep(&(struct timespec){0, 10 * ONE_MS}, NULL);
        }

        send_terminal_character(0x0d, true);
        send_terminal_character(0x0d, true);

        return true;
    }

    publish_message("\n\rFile not found\n\r", 18);
    return false;
}

/// <summary>
/// Altair terminal input callback
/// </summary>
static char terminal_read(void)
{
    char retVal;
    int ch;

    if (terminalInputCharacter)
    {
        retVal                 = terminalInputCharacter;
        terminalInputCharacter = 0x00;
        terminal_io_activity   = true;
        retVal &= 0x7F; // take first 7 bits (128 ascii chars)

        return retVal;
    }

    if (haveTerminalInputMessage)
    {
        retVal = input_data[altairInputBufReadIndex++];

        if (altairInputBufReadIndex >= terminalInputMessageLen)
        {
            haveTerminalInputMessage = false;
        }
        terminal_io_activity = true;
        retVal &= 0x7F; // take first 7 bits (128 ascii chars)
        return retVal;
    }

    if (haveAppLoad)
    {
        if ((read(app_fd, &ch, 1)) == 0)
        {
            close(app_fd);
            retVal                = 0x00;
            app_fd                = -1;
            terminal_read_pending = haveAppLoad = false;
        }
        else
        {
            retVal = (uint8_t)ch;
            if (retVal == '\n')
            {
                retVal = 0x0D;
            }
        }
        terminal_io_activity = true;
        retVal &= 0x7F; // take first 7 bits (128 ascii chars)
        return retVal;
    }
    return 0;
}

/// <summary>
/// Altair write Terminal output callback
/// </summary>
static void terminal_write(char c)
{
    c &= 0x7F;

    if (haveTerminalOutputMessage)
    {
        altairOutputBufReadIndex++;

        if (altairOutputBufReadIndex > terminalOutputMessageLen)
        {
            haveTerminalOutputMessage = false;
        }
    }
    else
    {
        terminal_io_activity = true;
        publish_character(c);
    }
}

static inline uint8_t sense(void)
{
    return (uint8_t)(bus_switches >> 8);
}

/// <summary>
/// Print welcome banner
/// </summary>
void print_console_banner(void)
{
    static bool first = true;
    // char reset[] = "\x1b[2J\r\n";
    const char reset[]          = "\r\n\r\n";
    const char altair_version[] = "\r\nAltair version: ";

    for (int x = 0; x < strlen(reset); x++)
    {
        terminal_write(reset[x]);
    }

    for (int x = 0; x < strlen(AltairMsg[AltairBannerCount]); x++)
    {
        terminal_write(AltairMsg[AltairBannerCount][x]);
    }

    AltairBannerCount++;
    AltairBannerCount = AltairBannerCount == sizeof(AltairMsg) / 4 ? 0 : AltairBannerCount;

    if (first)
    {
        first = false;

        for (int x = 0; x < strlen(altair_version); x++)
        {
            terminal_write(altair_version[x]);
        }

        for (int x = 0; x < strlen(ALTAIR_EMULATOR_VERSION); x++)
        {
            terminal_write(ALTAIR_EMULATOR_VERSION[x]);
        }
    }
    else
    {
        send_terminal_character(0x0d, true);
    }
}

/// <summary>
/// Intialize the Altair disks and i8080 cpu
/// </summary>
static void init_altair(void)
{
    // print_console_banner();

    memset(memory, 0x00, 64 * 1024); // clear memory.

    // initially no disk controller.
    disk_controller_t disk_controller;
    disk_controller.disk_function = disk_function;
    disk_controller.disk_select   = disk_select;
    disk_controller.disk_status   = disk_status;
    disk_controller.read          = disk_read;
    disk_controller.write         = disk_write;
    disk_controller.sector        = sector;

#ifndef SD_CARD_ENABLED
    disk_drive.disk1.fp = Storage_OpenFileInImagePackage(DISK_A_RO);
    if (disk_drive.disk1.fp == -1)
    {
        Log_Debug("Failed to load CPM Disk\n");
    }
#else
    disk_drive.disk1.fp          = -1;
    disk_drive.disk1.diskPointer = 0;
    disk_drive.disk1.sector      = 0;
    disk_drive.disk1.track       = 0;

    disk_drive.disk2.fp          = -1;
    disk_drive.disk2.diskPointer = 0;
    disk_drive.disk2.sector      = 0;
    disk_drive.disk2.track       = 0;

    disk_drive.disk3.fp          = -1;
    disk_drive.disk3.diskPointer = 0;
    disk_drive.disk3.sector      = 0;
    disk_drive.disk3.track       = 0;

    disk_drive.disk4.fp          = -1;
    disk_drive.disk4.diskPointer = 0;
    disk_drive.disk4.sector      = 0;
    disk_drive.disk4.track       = 0;

#endif

    i8080_reset(&cpu, (port_in)terminal_read, (port_out)terminal_write, sense, &disk_controller,
        (azure_sphere_port_in)io_port_in, (azure_sphere_port_out)io_port_out);

    // load Disk Loader at 0xff00
    if (!loadRomImage(DISK_LOADER, 0xff00))
        Log_Debug("Failed to load Disk ROM image\n");

    i8080_examine(&cpu, 0xff00); // 0xff00 loads from disk, 0x0000 loads basic
}

/// <summary>
/// Thread to run the i8080 cpu emulator on
/// </summary>
static void *altair_thread(void *arg)
{
    // Log_Debug("Altair Thread starting...\n");
    if (altair_i8080_running)
    {
        return NULL;
    }

    altair_i8080_running = true;

    while (!stop_cpu)
    {
        if (cpu_operating_mode == CPU_RUNNING)
        {
            i8080_cycle(&cpu);
        }

        if (send_partial_msg)
        {
            send_partial_message();
            send_partial_msg = false;
        }
    }

    altair_i8080_running = false;

    return NULL;
}

/// <summary>
/// Report on first connect the software version and device startup UTC time
/// </summary>
/// <param name="connected"></param>
static void report_startup_stats(bool connected)
{
    if (connected)
    {
        snprintf(msgBuffer, sizeof(msgBuffer), "Altair emulator version: %s, DevX version: %s",
            ALTAIR_EMULATOR_VERSION, AZURE_SPHERE_DEVX_VERSION);
        dx_deviceTwinReportValue(&dt_softwareVersion, msgBuffer);
        dx_deviceTwinReportValue(
            &dt_deviceStartTimeUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING

        dx_azureUnregisterConnectionChangedNotification(report_startup_stats);
    }
}

/// <summary>
/// Azure IoT connection state callback
/// </summary>
static void azure_connection_state(bool connection_state)
{
    azure_connected = connection_state;
}

/// <summary>
/// Start network interface and retry until enabled
/// </summary>
static void start_network_interface(void)
{
    int result, retry = 0;

    result = Networking_SetInterfaceState(network_interface, true);
    while (result == -1 && errno == EAGAIN && retry++ < 10)
    {
        nanosleep(&(struct timespec){0, 250 * ONE_MS}, NULL);
        result = Networking_SetInterfaceState(network_interface, true);
    }
}

/// <summary>
///  Initialize PeripheralGpios, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralAndHandlers(int argc, char *argv[])
{
    dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));

    parse_altair_cmd_line_arguments(argc, argv, &altair_config);

    network_interface = dx_isStringNullOrEmpty(altair_config.network_interface)
                            ? DEFAULT_NETWORK_INTERFACE
                            : altair_config.network_interface;

    start_network_interface();

    curl_global_init(CURL_GLOBAL_ALL);

    if (PowerManagement_SetSystemPowerProfile(PowerManagement_HighPerformance) == -1)
    {
        dx_Log_Debug("Setting power profile failed\n");
    }

    dx_gpioSetOpen(gpioSet, NELEMS(gpioSet));
    dx_i2cSetOpen(i2c_bindings, NELEMS(i2c_bindings));
    init_altair_hardware();

    dx_asyncSetInit(async_bindings, NELEMS(async_bindings));

    // No Azure IoT connection info configured so skip setting up connection
    if (altair_config.user_config.connectionType != DX_CONNECTION_TYPE_NOT_DEFINED)
    {
        dx_azureConnect(&altair_config.user_config, network_interface, IOT_PLUG_AND_PLAY_MODEL_ID);

        dx_azureRegisterConnectionChangedNotification(report_startup_stats);
        dx_azureRegisterConnectionChangedNotification(azure_connection_state);
    }

    dx_deviceTwinSubscribe(deviceTwinBindingSet, NELEMS(deviceTwinBindingSet));
    dx_directMethodSubscribe(directMethodBindingSet, NELEMS(directMethodBindingSet));

#ifdef OEM_AVNET
    avnet_open_adc(0);
    dx_intercoreConnect(&intercore_ml_classify_ctx);
#endif // OEM_AVNET

    init_web_socket_server(client_connected_cb);

    dx_timerSetStart(timerSet, NELEMS(timerSet));

    onboard_sensors_init(i2c_onboard_sensors.fd);

#ifdef SD_CARD_ENABLED

    dx_intercoreConnect(&intercore_sd_card_ctx);
    // set intercore read after publish timeout to 10000000 microseconds = 10 seconds
    dx_intercorePublishThenReadTimeout(&intercore_sd_card_ctx, 10000000);

    // Note, wifi_config needs up to 8KiB for certs and repurposes 64KiB memory allocated for the Altair
    // There wifi_config must not be called after the Altair in initialized
    wifi_config();

#else

    dx_intercoreConnect(&intercore_filesystem_ctx);
    // set intercore read after publish timeout to 1 second
    dx_intercorePublishThenReadTimeout(&intercore_filesystem_ctx, 10000000);

#endif // SD_CARD_ENABLED

    init_altair();
    dx_startThreadDetached(altair_thread, NULL, "altair_thread");
    while (!altair_i8080_running) // spin until i8080 thread starts
    {
        nanosleep(&(struct timespec){0, 1 * ONE_MS}, NULL);
    }

    // SetupWatchdog();
}

/// <summary>
///     Close PeripheralGpios and handlers.
/// </summary>
static void ClosePeripheralAndHandlers(void)
{
    dx_azureToDeviceStop();
    dx_deviceTwinUnsubscribe();
    dx_directMethodUnsubscribe();
    dx_timerEventLoopStop();
    dx_gpioSetClose(gpioSet, NELEMS(gpioSet));
    dx_i2cSetClose(i2c_bindings, NELEMS(i2c_bindings));
    onboard_sensors_close();
    curl_global_cleanup();
}

int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();
    InitPeripheralAndHandlers(argc, argv);

    // Blocking call to run main event loop until termination requested
    while (!terminationRequired)
    {
        if (asyncEventReady)
        {
            dx_asyncRunEvents();
        }

        int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR)
        {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        }
    }

    ClosePeripheralAndHandlers();
    Log_Debug("\n\nApplication exiting. Last known exit code: %d\n", dx_getTerminationExitCode());
    return dx_getTerminationExitCode();
}