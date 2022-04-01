#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "hw/altair.h"

#ifdef OEM_AVNET
#include "Drivers/AVNET/HL/imu_temp_pressure.h"
#endif

typedef struct {
    int temperature;
    int pressure;
	bool updated;
} ONBOARD_TELEMETRY;

bool onboard_sensors_read(ONBOARD_TELEMETRY *telemetry);
bool onboard_sensors_init(int i2c_fd);
bool onboard_sensors_close(void);
int onboard_get_pressure(void);
int onboard_get_temperature(void);
