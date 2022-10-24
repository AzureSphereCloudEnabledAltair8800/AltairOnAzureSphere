#pragma once

#include "altair_config.h"
#include "dx_intercore.h"
#include "intercore_contract.h"

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
#include "front_panel_retro_click.h"
#endif

extern uint8_t memory[64 * 1024];
extern ALTAIR_CONFIG_T altair_config;
extern DX_INTERCORE_BINDING intercore_sd_card_ctx;
extern DX_TIMER_BINDING tmr_display_ip_address;
extern DX_TIMER_BINDING tmr_clear_ip_address;
extern INTERCORE_DISK_DATA_BLOCK_T intercore_disk_block;
// extern const char DEFAULT_NETWORK_INTERFACE[];
extern const char *network_interface;

DX_DECLARE_TIMER_HANDLER(clear_ip_address_handler);
DX_DECLARE_TIMER_HANDLER(display_ip_address_handler);

void wifi_config(void);
void getIP(void);
