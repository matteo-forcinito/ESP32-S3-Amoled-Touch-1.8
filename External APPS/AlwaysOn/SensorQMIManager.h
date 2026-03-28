#pragma once
#include <Wire.h>
#include "SensorQMI8658.hpp"

class SensorQMIManager {
public:
    static SensorQMIManager& getInstance() {
        static SensorQMIManager instance;
        return instance;
    }

    bool begin() {
        if (!sensor.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
            return false;
        }

        // Config sensori con ODR compatibili I2C
        sensor.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                                   SensorQMI8658::ACC_ODR_125Hz,
                                   SensorQMI8658::LPF_MODE_0);
        sensor.configGyroscope(SensorQMI8658::GYR_RANGE_1024DPS,
                               SensorQMI8658::GYR_ODR_28_025Hz,
                               SensorQMI8658::LPF_MODE_0);

        // Abilita sensori
        sensor.enableAccelerometer();
        sensor.enableGyroscope();

        enabled = true;
        delay(10); // lascia un minimo tempo per inizializzazione
        return true;
    }

    void enable() {
        if (!enabled) {
            sensor.enableAccelerometer();
            sensor.enableGyroscope();
            enabled = true;
        }
    }

    void disable() {
        if (enabled) {
            sensor.disableAccelerometer();
            sensor.disableGyroscope();
            enabled = false;
        }
    }

    bool update() {
        if (!enabled || !sensor.getDataReady()) return false;
        sensor.getAccelerometer(accel.x, accel.y, accel.z);
        sensor.getGyroscope(gyro.x, gyro.y, gyro.z);
        gyro.x -= gyroOffsetX;
        gyro.y -= gyroOffsetY;
        gyro.z -= gyroOffsetZ;
        return true;
    }

    void calibrateGyro(uint16_t samples = 500, uint16_t delayMs = 5) {
        float sumX = 0, sumY = 0, sumZ = 0;
        IMUdata temp;
        for (uint16_t i = 0; i < samples; i++) {
            sensor.getGyroscope(temp.x, temp.y, temp.z);
            sumX += temp.x;
            sumY += temp.y;
            sumZ += temp.z;
            delay(delayMs);
        }
        gyroOffsetX = sumX / samples;
        gyroOffsetY = sumY / samples;
        gyroOffsetZ = sumZ / samples;
    }

    IMUdata getAccel() { return accel; }
    IMUdata getGyro() { return gyro; }
    bool isEnabled() { return enabled; }

private:
    SensorQMIManager() : enabled(false) {}
    SensorQMI8658 sensor;
    IMUdata accel;
    IMUdata gyro;
    bool enabled;

    float gyroOffsetX = 0;
    float gyroOffsetY = 0;
    float gyroOffsetZ = 0;
};