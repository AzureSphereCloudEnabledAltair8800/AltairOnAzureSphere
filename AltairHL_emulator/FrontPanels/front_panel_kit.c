/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "front_panel_kit.h"

#ifdef ALTAIR_FRONT_PANEL_KIT

int init_console(void) {
	// consoleFd
	UART_Config uartConfig;
	UART_InitConfig(&uartConfig);

	// Set the configuration
	uartConfig.baudRate = 115200;
	uartConfig.flowControl = UART_FlowControl_None;
	uartConfig.blockingMode = UART_BlockingMode_NonBlocking;

	return 0;
}

bool init_altair_hardware(void) {
	init_console();

	SPIMaster_Config config;
	SPIMaster_InitConfig(&config);

	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
	altair_spi_fd = SPIMaster_Open(MT3620_ISU1_SPI, MT3620_SPI_CS_A, &config);

	if (altair_spi_fd == -1) {
		dx_terminate(APP_EXIT_SEEED_SPI_OPEN);
		return false;
	}

	SPIMaster_SetBusSpeed(altair_spi_fd, 5000000);
	SPIMaster_SetBitOrder(altair_spi_fd, SPI_BitOrder_MsbFirst);
	SPIMaster_SetMode(altair_spi_fd, SPI_Mode_2);

	update_panel_status_leds(0xff, 0xff, 0xffff);
	nanosleep(&(struct timespec){0, 75 * ONE_MS}, NULL);
	update_panel_status_leds(0xaa, 0xaa, 0xaaaa);

	return true;
}

void read_switches(uint16_t* address, uint8_t* cmd) {
	if (altair_spi_fd == -1) { return; }

	uint32_t out = 0;
	dx_gpioStateSet(&switches_load, LOW);
	// delay(50);
	dx_gpioStateSet(&switches_load, HIGH);

	dx_gpioStateSet(&switches_chip_select, LOW);

	*address = 0;
	*cmd = 0;

	SPIMaster_Transfer transfer;
	SPIMaster_InitTransfers(&transfer, 1);
	transfer.flags = SPI_TransferFlags_Read;
	transfer.length = 3;
	transfer.readData = (uint8_t*)&out;

	ssize_t numRead = SPIMaster_TransferSequential(altair_spi_fd, &transfer, 1);

	if (numRead == transfer.length) {
		//read(altair_spi_fd, &out, 3);

		*cmd = (out >> 16) & 0xff;

		*address = out & 0xffff;
		*address = reverse_lut[(*address & 0xf000) >> 12] << 8 | reverse_lut[(*address & 0x0f00) >> 8] << 12 | reverse_lut[(*address & 0xf0) >> 4] | reverse_lut[*address & 0xf] << 4;
        *address = (uint16_t)~*address;
	} else {
		dx_terminate(APP_EXIT_SEEED_SPI_TRANSFER);
	}
	dx_gpioStateSet(&switches_chip_select, HIGH);
}

void read_altair_panel_switches(void (*process_control_panel_commands)(void))
{
	static ALTAIR_COMMAND last_command = NOP;
	//static uint16_t last_address       = 0;

	uint16_t address = 0;
	uint8_t cmd      = 0;

	read_switches(&address, &cmd);

	bus_switches = address;

	//if (address != last_address)
	//{
	//	last_address = address;
	//	bus_switches = address;
	//}

	if (cmd != last_command)
	{
		last_command = cmd;
		cmd_switches = cmd;
		process_control_panel_commands();
	}
}

void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus) {
	if (altair_spi_fd == -1) { return; }

	uint32_t out = status << 24 | data << 16 | bus;

	// store LED bits.
	dx_gpioStateSet(&led_store, LOW);

	SPIMaster_Transfer transfer;
	SPIMaster_InitTransfers(&transfer, 1);
	transfer.flags = SPI_TransferFlags_Write;
	transfer.length = 4;
    transfer.writeData = (const uint8_t *)&out;

	ssize_t numWrite = SPIMaster_TransferSequential(altair_spi_fd, &transfer, 1);
	if (numWrite != transfer.length) {
		//Log_Debug("Error transferring Altair Front panel LED bytes");
	}
	dx_gpioStateSet(&led_store, HIGH);
}

#endif // ALTAIR_FRONT_PANEL_KIT

DX_TIMER_HANDLER(turn_off_notifications_handler)
{
}
DX_TIMER_HANDLER_END