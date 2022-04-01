# Configuring SD Card Service

## SD Card Mikroe Click

Must be placed in Socket 1 for both the Rev 1 and 2 boards

## Update App Manifest

## Update main.c

| Avnet Board | Click Socket | ISU Number | Chip select | app_manifest |
|-------|--------|------------|-------------|------|
| Rev 1 | Socket 1 | ISU1 | 0 | ``` "SpiMaster": [ "ISU1" ] ``` |
| Rev 2 | Socket 1 | ISU0 | 0 | ``` "SpiMaster": [ "ISU0" ] ``` |

```c
driver = SPIMaster_Open(MT3620_UNIT_ISU1);
if (!driver) {
    UART_Print(debug,
        "ERROR: SPI initialisation failed\r\n");
}
SPIMaster_DMAEnable(driver, false);

// Use CSA for chip select.
SPIMaster_Select(driver, 0);
```
