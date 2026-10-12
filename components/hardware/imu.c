#include "hardware/imu.h"
#include "hardware/i2c_bus.h"
#include "board_config.h"
#include "qmi8658.h"

static qmi8658_t s_sensor;
bool imu_supported(void)
{
    return true;
}
esp_err_t imu_start(void)
{
    return qmi8658_start(&s_sensor, i2c_bus_handle(), BOARD_IMU_ADDR);
}
esp_err_t imu_read(imu_sample_t *sample)
{
    return qmi8658_read(&s_sensor, sample);
}
esp_err_t imu_stop(void)
{
    return qmi8658_stop(&s_sensor);
}
