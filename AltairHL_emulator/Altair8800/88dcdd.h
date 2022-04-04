#ifndef _88DCDD_H_
#define _88DCDD_H_

#include "app_exit_codes.h"
#include "dx_device_twins.h"
#include "dx_intercore.h"
#include "intercore_contract.h"

extern DX_INTERCORE_BINDING intercore_filesystem_ctx;


#define STATUS_ENWD			1
#define STATUS_MOVE_HEAD	2
#define STATUS_HEAD			4
#define STATUS_IE			32
#define STATUS_TRACK_0		64
#define STATUS_NRDA			128

#define CONTROL_STEP_IN		1
#define CONTROL_STEP_OUT	2
#define CONTROL_HEAD_LOAD	4
#define CONTROL_HEAD_UNLOAD 8
#define CONTROL_IE			16
#define CONTROL_ID			32
#define CONTROL_HCS			64
#define CONTROL_WE			128

#define SECTOR_SIZE 137UL
#define TRACK (32UL*SECTOR_SIZE)

typedef struct
{
	int		fp;
	uint8_t track;
	uint8_t sector;
	uint8_t status;
	uint8_t write_status;
	uint32_t diskPointer;
	uint8_t sectorPointer;
	uint8_t sectorData[SECTOR_SIZE + 2];
	bool sectorDirty;
	bool haveSectorData;
} disk_t;


typedef struct
{
	disk_t disk1;
#ifdef SD_CARD_ENABLED
	disk_t disk2;
#endif // SD_CARD_ENABLED
	disk_t nodisk;
	disk_t *current;
	uint8_t currentDisk;
} disks;

extern disks disk_drive;
extern DX_INTERCORE_BINDING intercore_sd_card_ctx;
extern INTERCORE_DISK_DATA_BLOCK_T intercore_disk_block;
extern DX_DEVICE_TWIN_BINDING dt_difference_disk_reads;
extern DX_DEVICE_TWIN_BINDING dt_difference_disk_writes;
extern DX_DEVICE_TWIN_BINDING dt_filesystem_reads;

void disk_select(uint8_t b);
uint8_t disk_status(void);
void disk_function(uint8_t b);
uint8_t sector(void);
void disk_write(uint8_t b);
uint8_t disk_read(void);
void clear_difference_disk(void);

typedef struct {
	int sector_number;
	bool dirty;
	uint8_t data[137];
} VDISK_SECTOR_T;

#endif
