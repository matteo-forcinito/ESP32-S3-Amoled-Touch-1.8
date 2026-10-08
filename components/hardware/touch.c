#include "hardware/touch.h"

#include "hardware/i2c_bus.h"

#include "board_config.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "touch";

/* FT3168 registers (FocalTech family, same map as FT5x06/FT6x36). */
#define REG_TD_STATUS   0x02   /* number of touch points + 4 bytes of point 1 */
#define REG_G_MODE      0xA4   /* 0 = INT low while touched, 1 = pulse */
#define REG_POWER_MODE  0xA5   /* 0 = active, 1 = monitor */
#define REG_G_CTRL      0x86   /* 1 = switch to monitor by itself when idle */
#define REG_MONITOR_TIME 0x87  /* seconds without touch before monitor mode */

static i2c_master_dev_handle_t s_dev = NULL;
static touch_irq_cb_t s_on_touch = NULL;
static void *s_ctx = NULL;

static void touch_isr(void *arg)
{
    (void)arg;

    /* Level interrupt: disable it, or it fires again until the finger lifts. */
    gpio_intr_disable(BOARD_TOUCH_PIN_INT);

    if (s_on_touch != NULL)
    {
        s_on_touch(s_ctx);
    }
}

esp_err_t touch_init(touch_irq_cb_t on_touch, void *ctx)
{
    s_on_touch = on_touch;
    s_ctx = ctx;

    esp_err_t err = i2c_bus_add(BOARD_TOUCH_ADDR, &s_dev);

    if (err != ESP_OK)
    {
        return err;
    }

    if (i2c_bus_write_byte(s_dev, REG_G_MODE, 0x00) != ESP_OK)
    {
        ESP_LOGW(TAG, "FT3168 not answering (it may be asleep, will retry on touch)");
    }

    /* The chip slows its own scanning down after 2 s without touches. */
    i2c_bus_write_byte(s_dev, REG_G_CTRL, 0x01);
    i2c_bus_write_byte(s_dev, REG_MONITOR_TIME, 2);

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BOARD_TOUCH_PIN_INT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_LOW_LEVEL,
    };
    gpio_config(&io);
    gpio_isr_handler_add(BOARD_TOUCH_PIN_INT, touch_isr, NULL);

    /* A finger on the glass wakes the chip from light sleep. */
    gpio_wakeup_enable(BOARD_TOUCH_PIN_INT, GPIO_INTR_LOW_LEVEL);

    ESP_LOGI(TAG, "FT3168 ready (INT on GPIO %d)", BOARD_TOUCH_PIN_INT);

    return ESP_OK;
}

bool touch_read(uint16_t *x, uint16_t *y)
{
    uint8_t data[5];

    if (s_dev == NULL || i2c_bus_read(s_dev, REG_TD_STATUS, data, sizeof(data)) != ESP_OK)
    {
        return false;
    }

    uint8_t points = data[0] & 0x0F;

    if (points == 0 || points > 2)
    {
        return false;
    }

    uint16_t px = (uint16_t)(((data[1] & 0x0F) << 8) | data[2]);
    uint16_t py = (uint16_t)(((data[3] & 0x0F) << 8) | data[4]);

    if (px >= BOARD_LCD_WIDTH)
    {
        px = BOARD_LCD_WIDTH - 1;
    }

    if (py >= BOARD_LCD_HEIGHT)
    {
        py = BOARD_LCD_HEIGHT - 1;
    }

    *x = px;
    *y = py;

    return true;
}

bool touch_pin_active(void)
{
    return gpio_get_level(BOARD_TOUCH_PIN_INT) == 0;
}

void touch_irq_arm(void)
{
    gpio_intr_enable(BOARD_TOUCH_PIN_INT);
}

esp_err_t touch_set_low_power(bool low_power)
{
    if (s_dev == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return i2c_bus_write_byte(s_dev, REG_POWER_MODE, low_power ? 0x01 : 0x00);
}
