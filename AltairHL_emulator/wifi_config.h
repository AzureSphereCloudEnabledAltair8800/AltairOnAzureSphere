#pragma once

#include "../IntercoreContract/intercore_contract.h"
#include "dx_intercore.h"
#include "dx_utilities.h"
#include "parson.h"
#include <applibs/log.h>
#include <applibs/wificonfig.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <sys/socket.h>

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
#include "front_panel_retro_click.h"
#endif

extern ALTAIR_CONFIG_T altair_config;
extern DX_INTERCORE_BINDING intercore_sd_card_ctx;
extern DX_TIMER_BINDING tmr_display_ip_address;
extern INTERCORE_DISK_DATA_BLOCK_T intercore_disk_block;
extern const char DEFAULT_NETWORK_INTERFACE[];

DX_DECLARE_TIMER_HANDLER(display_ip_address_handler);

void wifi_config(void);
void getIP(void);
