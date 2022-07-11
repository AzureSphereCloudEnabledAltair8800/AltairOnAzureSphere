#include "88dcdd.h"

#define DISK_DEBUG

typedef enum
{
	TRACK_MODE,
	SECTOR_MODE
} DISK_SELECT_MODE;
void writeSector(disk_t *pDisk, uint8_t drive_number);
disks disk_drive;

void set_status(uint8_t bit)
{
	disk_drive.current->status &= (uint8_t)~bit;
}

void clear_status(uint8_t bit)
{
	disk_drive.current->status |= bit;
}

void disk_select(uint8_t b)
{
	uint8_t select         = b & 0xf;
	disk_drive.currentDisk = select;

	switch (select)
	{
		case 0:
			disk_drive.current = &disk_drive.disk1;
			break;

#ifdef SD_CARD_ENABLED
		case 1:
			disk_drive.current = &disk_drive.disk2;
			break;
		case 2:
			disk_drive.current = &disk_drive.disk3;
			break;
		case 3:
			disk_drive.current = &disk_drive.disk4;
			break;
#endif // SD_CARD_ENABLED

		default:
			disk_drive.current     = &disk_drive.disk1;
			disk_drive.currentDisk = 0;
			break;
	}
}

uint8_t disk_status()
{
	return disk_drive.current->status;
}

void disk_function(uint8_t b)
{
	if (b & CONTROL_STEP_IN)
	{
		disk_drive.current->track++;
		disk_drive.current->sector = 0;

		if (disk_drive.current->track != 0)
		{
			clear_status(STATUS_TRACK_0);
		}

		uint32_t seek_offset = TRACK * disk_drive.current->track;

#ifndef SD_CARD_ENABLED
		lseek(disk_drive.current->fp, seek_offset, SEEK_SET);
#endif

		if (disk_drive.current->sectorDirty)
		{
			writeSector(disk_drive.current, disk_drive.currentDisk);
		}

		disk_drive.current->diskPointer    = seek_offset;
		disk_drive.current->haveSectorData = false;
		disk_drive.current->sectorPointer  = 0;
	}

	if (b & CONTROL_STEP_OUT)
	{
		if (disk_drive.current->track > 0)
		{
			disk_drive.current->track--;
		}

		if (disk_drive.current->track == 0)
		{
			set_status(STATUS_TRACK_0);
		}

		disk_drive.current->sector = 0;
		uint32_t seek_offset       = TRACK * disk_drive.current->track;

#ifndef SD_CARD_ENABLED
		lseek(disk_drive.current->fp, seek_offset, SEEK_SET);
#endif

		if (disk_drive.current->sectorDirty)
		{
			writeSector(disk_drive.current, disk_drive.currentDisk);
		}

		disk_drive.current->diskPointer    = seek_offset;
		disk_drive.current->haveSectorData = false;
		disk_drive.current->sectorPointer  = 0;
	}

	if (b & CONTROL_HEAD_LOAD)
	{
		set_status(STATUS_HEAD);
		set_status(STATUS_NRDA);
	}

	if (b & CONTROL_HEAD_UNLOAD)
	{
		clear_status(STATUS_HEAD);
	}

	if (b & CONTROL_IE)
	{
	}

	if (b & CONTROL_ID)
	{
	}

	if (b & CONTROL_HCS)
	{
	}

	if (b & CONTROL_WE)
	{
		set_status(STATUS_ENWD);
		disk_drive.current->write_status = 0;
	}
}

uint8_t sector()
{
	uint32_t seek_offset;
	uint8_t ret_val;

	if (disk_drive.current->sector == 32)
	{
		disk_drive.current->sector = 0;
	}

	if (disk_drive.current->sectorDirty)
	{
		writeSector(disk_drive.current, disk_drive.currentDisk);
	}

	seek_offset = disk_drive.current->track * TRACK + disk_drive.current->sector * (SECTOR_SIZE);
	disk_drive.current->sectorPointer = 0;

#ifndef SD_CARD_ENABLED
	lseek(disk_drive.current->fp, seek_offset, SEEK_SET);
#endif

	// needs to be set here for write operation (read fetches sector data and resets the pointer).
	disk_drive.current->diskPointer    = seek_offset;
	disk_drive.current->sectorPointer  = 0;
	disk_drive.current->haveSectorData = false;

	ret_val = (uint8_t)(disk_drive.current->sector << 1);

	disk_drive.current->sector++;
	return ret_val;
}

void disk_write(uint8_t b)
{
	disk_drive.current->sectorData[disk_drive.current->sectorPointer++] = b;
	disk_drive.current->sectorDirty                                     = true;

	if (disk_drive.current->write_status == 137)
	{
		writeSector(disk_drive.current, disk_drive.currentDisk);

		disk_drive.current->write_status = 0;
		clear_status(STATUS_ENWD);
	}
	else
		disk_drive.current->write_status++;
}

uint8_t disk_read()
{
	uint16_t requested_sector_number = (uint16_t)(disk_drive.current->diskPointer / 137);

#ifdef SD_CARD_ENABLED
	if (!disk_drive.current->haveSectorData)
	{
		disk_drive.current->sectorPointer = 0;

		memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));
		intercore_disk_block.cached           = false;
		intercore_disk_block.success          = false;
		intercore_disk_block.drive_number     = disk_drive.currentDisk;
		intercore_disk_block.sector_number    = requested_sector_number;
		intercore_disk_block.disk_ic_msg_type = DISK_IC_READ;

		dx_intercorePublishThenRead(
			&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

		if (intercore_disk_block.success)
		{
			disk_drive.current->haveSectorData = true;
			memcpy(disk_drive.current->sectorData, intercore_disk_block.sector, 137);
		}
	}
#else

	if (!disk_drive.current->haveSectorData)
	{
		memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));
		intercore_disk_block.disk_ic_msg_type = DISK_IC_READ;
		intercore_disk_block.sector_number    = requested_sector_number;
		intercore_disk_block.cached           = false;
		intercore_disk_block.success          = false;

		ssize_t bytes_returned =
			dx_intercorePublishThenRead(&intercore_filesystem_ctx, &intercore_disk_block, 3);

		if (bytes_returned == -1)
		{
			Log_Debug("AltairRT_filesystem_ext real-time app has not been deployed\n");
			dx_terminate(APP_EXIT_REAL_TIME_FILESYSTEM_EXT_NOT_DEPLOYED);
		}

		if (bytes_returned > 0 && intercore_disk_block.cached &&
			intercore_disk_block.sector_number == requested_sector_number)
		{
			disk_drive.current->sectorPointer  = 0;
			disk_drive.current->haveSectorData = true;
			memcpy(disk_drive.current->sectorData, intercore_disk_block.sector, SECTOR_SIZE);
			(*(int *)dt_difference_disk_reads.propertyValue)++;
		}
		else
		{
			disk_drive.current->sectorPointer  = 0;
			disk_drive.current->haveSectorData = true;
			read(disk_drive.current->fp, disk_drive.current->sectorData, SECTOR_SIZE);
			(*(int *)dt_filesystem_reads.propertyValue)++;
		}
	}

#endif // SD_CARD_ENABLED

	return disk_drive.current->sectorData[disk_drive.current->sectorPointer++];
}

void writeSector(disk_t *pDisk, uint8_t drive_number)
{
	uint16_t requested_sector_number = (uint16_t)(pDisk->diskPointer / SECTOR_SIZE);

#ifdef SD_CARD_ENABLED

	memcpy(intercore_disk_block.sector, pDisk->sectorData, 137);
	intercore_disk_block.cached           = false;
	intercore_disk_block.success          = false;
	intercore_disk_block.drive_number     = drive_number;
	intercore_disk_block.sector_number    = requested_sector_number;
	intercore_disk_block.disk_ic_msg_type = DISK_IC_WRITE;

	dx_intercorePublishThenRead(&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

#else

	memcpy(intercore_disk_block.sector, pDisk->sectorData, SECTOR_SIZE);
	intercore_disk_block.sector_number    = requested_sector_number;
	intercore_disk_block.disk_ic_msg_type = DISK_IC_WRITE;
	dx_intercorePublish(&intercore_filesystem_ctx, &intercore_disk_block, sizeof(intercore_disk_block));
	(*(int *)dt_difference_disk_writes.propertyValue)++;

#endif // SD_CARD_ENABLED

	pDisk->sectorPointer = 0;
	pDisk->sectorDirty   = false;
}

void clear_difference_disk(void)
{
	intercore_disk_block.disk_ic_msg_type = DISK_IC_CLEAR;
	dx_intercorePublish(&intercore_filesystem_ctx, &intercore_disk_block, sizeof(INTERCORE_DISK_MSG_TYPE));
}
