#include "hardware/board.h"

#include "hardware/i2c_bus.h"
#include "hardware/pmu.h"
#include "hardware/rtc.h"
#include "hardware/sdcard.h"

#include "board_config.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "board";

static const board_info_t s_info = {
    .name = BOARD_NAME,
    .width = BOARD_LCD_WIDTH,
    .height = BOARD_LCD_HEIGHT,
    .round = BOARD_LCD_ROUND,
    .has_touch = BOARD_HAS_TOUCH,
    .has_pmu = BOARD_HAS_PMU,
    .has_rtc = BOARD_HAS_RTC,
    .has_audio = BOARD_HAS_AUDIO,
    .has_sdcard = BOARD_HAS_SDCARD,
};

#if BOARD_HAS_IO_EXPANDER

/* TCA9554 registers */
#define EXP_REG_OUTPUT  0x01
#define EXP_REG_CONFIG  0x03   /* 1 = input, 0 = output */

/*
 * The display and touch reset lines hang on the IO expander. At power-up its
 * pins are inputs (pulled high, so the chips run), but after a soft restart
 * the panel may be in a strange state: pulse the resets once.
 */
static void io_expander_reset_pulse(void)
{
    if (!i2c_bus_probe(BOARD_IO_EXPANDER_ADDR))
    {
        ESP_LOGW(TAG, "IO expander not found, skipping reset pulse");
        return;
    }

    i2c_master_dev_handle_t dev = NULL;

    if (i2c_bus_add(BOARD_IO_EXPANDER_ADDR, &dev) != ESP_OK)
    {
        return;
    }

    uint8_t config = 0xFF;
    uint8_t output = 0xFF;
    i2c_bus_read_byte(dev, EXP_REG_CONFIG, &config);
    i2c_bus_read_byte(dev, EXP_REG_OUTPUT, &output);

    i2c_bus_write_byte(dev, EXP_REG_OUTPUT, output & ~BOARD_IO_EXPANDER_RESET_MASK);
    i2c_bus_write_byte(dev, EXP_REG_CONFIG, config & ~BOARD_IO_EXPANDER_RESET_MASK);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_bus_write_byte(dev, EXP_REG_OUTPUT, output | BOARD_IO_EXPANDER_RESET_MASK);
    vTaskDelay(pdMS_TO_TICKS(120));

    i2c_master_bus_rm_device(dev);
}

#endif

/*
 * In automatic light sleep ESP-IDF "isolates" every GPIO (no output, no pull)
 * to save ~250 uA. The pins below must keep their state instead: the panel
 * chip select must stay high, the I2C and SD lines idle high, the amplifier
 * enable low (otherwise it may switch on by itself), and the wake-up inputs
 * (touch INT, BOOT) keep their pull-ups.
 */
static void keep_pins_in_light_sleep(void)
{
    static const int pins[] = {
        BOARD_LCD_PIN_CS, BOARD_LCD_PIN_SCLK,
        BOARD_I2C_PIN_SDA, BOARD_I2C_PIN_SCL,
        BOARD_PIN_BUTTON_BOOT,
#if BOARD_HAS_TOUCH
        BOARD_TOUCH_PIN_INT,
#endif
#if BOARD_HAS_AUDIO
        BOARD_AUDIO_PIN_PA,
#endif
#if BOARD_HAS_SDCARD
        BOARD_SD_PIN_CLK, BOARD_SD_PIN_CMD, BOARD_SD_PIN_D0,
#endif
    };

    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        if (pins[i] >= 0)
        {
            gpio_sleep_sel_dis((gpio_num_t)pins[i]);
        }
    }
}

esp_err_t board_init(void)
{
    ESP_LOGI(TAG, "%s (%ux%u)", s_info.name, s_info.width, s_info.height);

    esp_err_t err = i2c_bus_init();

    if (err != ESP_OK)
    {
        return err;
    }

    /* One shared GPIO interrupt service for touch INT and the BOOT button. */
    err = gpio_install_isr_service(0);

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }

    /* GPIO levels (touch INT, BOOT) wake the chip from automatic light sleep. */
    esp_sleep_enable_gpio_wakeup();
    keep_pins_in_light_sleep();

#if BOARD_HAS_AUDIO
    /* Amplifier off until audio is used (the codec driver drives it later). */
    gpio_config_t pa = {.pin_bit_mask = 1ULL << BOARD_AUDIO_PIN_PA, .mode = GPIO_MODE_OUTPUT};
    gpio_config(&pa);
    gpio_set_level(BOARD_AUDIO_PIN_PA, 0);
#endif

#if BOARD_HAS_IO_EXPANDER
    io_expander_reset_pulse();
#endif

#if BOARD_HAS_PMU
    if (pmu_init() != ESP_OK)
    {
        ESP_LOGW(TAG, "PMU not answering");
    }
#endif

#if BOARD_HAS_RTC
    if (rtc_chip_init() != ESP_OK)
    {
        ESP_LOGW(TAG, "RTC not answering");
    }
#endif

#if BOARD_HAS_SDCARD
    if (sdcard_mount() != ESP_OK)
    {
        ESP_LOGW(TAG, "No SD card");
    }
#endif

    return ESP_OK;
}

const board_info_t *board_info(void)
{
    return &s_info;
}
