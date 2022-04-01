/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "dx_timer.h"


DX_DECLARE_TIMER_HANDLER(turn_off_notifications_handler);


bool init_altair_hardware(void);
void read_altair_panel_switches(void (*process_control_panel_commands)(void));
void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);