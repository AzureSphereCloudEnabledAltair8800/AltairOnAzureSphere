/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

#include "app_exit_codes.h"
#include "web_socket_server.h"
#include "dx_gpio.h"
#include "dx_timer.h"
#include "dx_i2c.h"
#include "graphics.h"
#include "hw/azure_sphere_learning_path.h"
#include "as1115.h"
#include "altair_panel.h"
#include "utils.h"
#include <applibs/gpio.h>
#include <stdio.h>


extern volatile ALTAIR_COMMAND cmd_switches;
extern bool renderText;
extern CLICK_4X4_BUTTON_MODE click_4x4_key_mode;
extern volatile CPU_OPERATING_MODE cpu_operating_mode;
extern int console_fd;
as1115_t retro_click;
extern DX_GPIO_BINDING buttonB;
extern DX_TIMER_BINDING tmr_turn_off_notifications;
extern volatile uint16_t bus_switches;
extern DX_I2C_BINDING i2c_as1115_retro;


void check_click_4x4key_mode_button(void);
DX_DECLARE_TIMER_HANDLER(turn_off_notifications_handler);
void init_altair_hardware(void);
void read_altair_panel_switches(void (*process_control_panel_commands)(void));
void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK