#ifndef HARDWARE_I2C_BUS_H
#define HARDWARE_I2C_BUS_H

/*
 * The shared I2C bus. Every chip driver adds itself as a "device" on it
 * (the ESP-IDF i2c_master driver serializes access between tasks).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t i2c_bus_init(void);

i2c_master_bus_handle_t i2c_bus_handle(void);

/* True if a chip answers at `address`. */
bool i2c_bus_probe(uint8_t address);

/* Add a device at 400 kHz. */
esp_err_t i2c_bus_add(uint8_t address, i2c_master_dev_handle_t *out);

/* Register helpers (8-bit register addresses). */
esp_err_t i2c_bus_read(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *data, size_t length);
esp_err_t i2c_bus_write(i2c_master_dev_handle_t dev, uint8_t reg, const uint8_t *data, size_t length);
esp_err_t i2c_bus_read_byte(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *value);
esp_err_t i2c_bus_write_byte(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value);

#endif
