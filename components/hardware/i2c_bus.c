#include "hardware/i2c_bus.h"

#include "board_config.h"

#include "esp_log.h"

#define I2C_PORT        I2C_NUM_0
#define I2C_SPEED_HZ    400000
#define I2C_TIMEOUT_MS  50

static const char *TAG = "i2c_bus";

static i2c_master_bus_handle_t s_bus = NULL;

esp_err_t i2c_bus_init(void)
{
    if (s_bus != NULL)
    {
        return ESP_OK;
    }

    i2c_master_bus_config_t config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = BOARD_I2C_PIN_SDA,
        .scl_io_num = BOARD_I2C_PIN_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&config, &s_bus);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Init failed: %s", esp_err_to_name(err));
    }

    return err;
}

i2c_master_bus_handle_t i2c_bus_handle(void)
{
    return s_bus;
}

bool i2c_bus_probe(uint8_t address)
{
    return s_bus != NULL && i2c_master_probe(s_bus, address, I2C_TIMEOUT_MS) == ESP_OK;
}

esp_err_t i2c_bus_add(uint8_t address, i2c_master_dev_handle_t *out)
{
    if (s_bus == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = I2C_SPEED_HZ,
    };

    return i2c_master_bus_add_device(s_bus, &config, out);
}

esp_err_t i2c_bus_read(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *data, size_t length)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, length, I2C_TIMEOUT_MS);
}

esp_err_t i2c_bus_write(i2c_master_dev_handle_t dev, uint8_t reg, const uint8_t *data, size_t length)
{
    uint8_t buffer[16];

    if (length + 1 > sizeof(buffer))
    {
        return ESP_ERR_INVALID_SIZE;
    }

    buffer[0] = reg;

    for (size_t i = 0; i < length; i++)
    {
        buffer[i + 1] = data[i];
    }

    return i2c_master_transmit(dev, buffer, length + 1, I2C_TIMEOUT_MS);
}

esp_err_t i2c_bus_read_byte(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *value)
{
    return i2c_bus_read(dev, reg, value, 1);
}

esp_err_t i2c_bus_write_byte(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value)
{
    return i2c_bus_write(dev, reg, &value, 1);
}
