#pragma once

#include "../IntercoreContract/intercore_contract.h"
#include "dx_intercore.h"
#include "dx_utilities.h"
#include "parson.h"
#include <applibs/log.h>
#include <applibs/wificonfig.h>

extern DX_INTERCORE_BINDING intercore_sd_card_ctx;
extern INTERCORE_DISK_DATA_BLOCK_T intercore_disk_block;

void wifi_config(void);
