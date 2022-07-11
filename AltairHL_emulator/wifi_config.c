#include "wifi_config.h"

void wifi_config(void)
{
	memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));
	intercore_disk_block.cached           = false;
	intercore_disk_block.success          = false;
	intercore_disk_block.drive_number     = 5;
	intercore_disk_block.sector_number    = 0;
	intercore_disk_block.disk_ic_msg_type = DISK_IC_READ;

	dx_intercorePublishThenRead(&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

	if (intercore_disk_block.success)
	{
		dx_Log_Debug(intercore_disk_block.sector);
	}
}
