#include "onboard_sensors.h"

#ifdef OEM_AVNET

bool onboard_sensors_init(int i2c_fd) {
    srand((unsigned int)time(NULL)); // seed the random number generator for fake telemetry
    avnet_imu_initialize(i2c_fd);

    // lp_calibrate_angular_rate(); // call if using gyro
    // lp_OpenADC();

    return true;
}

int onboard_get_temperature(void)
{
	return (int)avnet_get_temperature();
}

int onboard_get_pressure(void)
{
	return (int)avnet_get_pressure();
}

/// <summary>
///     Reads telemetry from Avnet onboard sensors
/// </summary>
bool onboard_sensors_read(ONBOARD_TELEMETRY *telemetry)
{
    // ENSURE lp_calibrate_angular_rate(); call from lp_initializeDevKit before calling lp_get_angular_rate()

    // AngularRateDegreesPerSecond ardps = lp_get_angular_rate();
    // AccelerationMilligForce amf = lp_get_acceleration();

    // Log_Debug("\nLSM6DSO: Angular rate [degrees per second] : %4.2f, %4.2f, %4.2f\n", ardps.x, ardps.y, ardps.z);
    // Log_Debug("\nLSM6DSO: Acceleration [millig force]  : %.4lf, %.4lf, %.4lf\n", amgf.x, amgf.y, amgf.z);
    // telemetry->light = lp_GetLightLevel();

    telemetry->temperature = (int)onboard_get_temperature();
    telemetry->pressure = (int)onboard_get_pressure();

    return true;
}

bool onboard_sensors_close(void) {
    return true;
}


#else

bool onboard_sensors_init(int i2c_fd)
{
    srand((unsigned int)time(NULL)); // seed the random number generator for fake telemetry
    return true;
}

int onboard_get_temperature(void)
{
	int rnd                = (rand() % 10) - 5;
	return 25 + rnd;
}

int onboard_get_pressure(void)
{
	int rnd = (rand() % 50) - 25;
	return 1000 + rnd;
}

/// <summary>
///     Generate fake telemetry for Seeed Studio dev boards
/// </summary>
bool onboard_sensors_read(ONBOARD_TELEMETRY *telemetry)
{
	telemetry->temperature = onboard_get_temperature();
	telemetry->pressure    = onboard_get_pressure();

    return true;
}

bool onboard_sensors_close(void)
{
    return true;
}

#endif // OEM_AVNET
