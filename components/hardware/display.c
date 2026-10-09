#include "hardware/display.h"

#include "board_config.h"

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_sh8601.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";

#define LCD_HOST          SPI2_HOST
#define LCD_OPCODE_WRITE  0x02u

/* SH8601 commands we send ourselves (the driver covers the rest). */
#define CMD_SLEEP_IN      0x10
#define CMD_SLEEP_OUT     0x11
#define CMD_DISPLAY_OFF   0x28
#define CMD_DISPLAY_ON    0x29
#define CMD_BRIGHTNESS    0x51

/*
 * Panel start-up sequence (from Waveshare's 1.8" examples). Brightness
 * starts at 0: the first frame is drawn while the screen is still dark,
 * then the power manager turns the light on, so no garbage is ever shown.
 */
static const sh8601_lcd_init_cmd_t s_init_cmds[] = {
    {CMD_SLEEP_OUT, (uint8_t[]){0x00}, 0, 120},
    {0x44, (uint8_t[]){0x01, 0xD1}, 2, 0},                   /* tear scan line */
    {0x35, (uint8_t[]){0x00}, 1, 0},                         /* tearing effect on */
    {0x53, (uint8_t[]){0x20}, 1, 10},                        /* brightness control on */
    {0x2A, (uint8_t[]){0x00, 0x00, 0x01, 0x6F}, 4, 0},       /* columns 0..367 */
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xBF}, 4, 0},       /* rows 0..447 */
    {CMD_BRIGHTNESS, (uint8_t[]){0x00}, 1, 10},
    {CMD_DISPLAY_ON, (uint8_t[]){0x00}, 0, 10},
};

static esp_lcd_panel_io_handle_t s_io = NULL;
static esp_lcd_panel_handle_t s_panel = NULL;
static display_done_cb_t s_done = NULL;
static void *s_done_ctx = NULL;
static bool s_sleeping = false;

static bool on_transfer_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *data, void *ctx)
{
    (void)io;
    (void)data;
    (void)ctx;

    if (s_done != NULL)
    {
        s_done(s_done_ctx);
    }

    return false;
}

/* QSPI framing: opcode 0x02 + command in the middle byte of a 32-bit "command". */
static esp_err_t send_command(uint8_t command, const uint8_t *data, size_t length)
{
    int lcd_cmd = (int)((LCD_OPCODE_WRITE << 24) | ((uint32_t)command << 8));
    return esp_lcd_panel_io_tx_param(s_io, lcd_cmd, data, length);
}

esp_err_t display_init(display_done_cb_t done, void *ctx)
{
    s_done = done;
    s_done_ctx = ctx;

    const size_t max_transfer = BOARD_LCD_WIDTH * CONFIG_BOARD_DRAW_BUFFER_LINES * sizeof(uint16_t);
    const spi_bus_config_t bus_config = SH8601_PANEL_BUS_QSPI_CONFIG(
        BOARD_LCD_PIN_SCLK, BOARD_LCD_PIN_D0, BOARD_LCD_PIN_D1,
        BOARD_LCD_PIN_D2, BOARD_LCD_PIN_D3, max_transfer);

    esp_err_t err = spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI bus: %s", esp_err_to_name(err));
        return err;
    }

    esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(BOARD_LCD_PIN_CS, on_transfer_done, NULL);
    io_config.pclk_hz = CONFIG_BOARD_LCD_PCLK_MHZ * 1000 * 1000;

    err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_io);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Panel IO: %s", esp_err_to_name(err));
        return err;
    }

    sh8601_vendor_config_t vendor_config = {
        .init_cmds = s_init_cmds,
        .init_cmds_size = sizeof(s_init_cmds) / sizeof(s_init_cmds[0]),
        .flags.use_qspi_interface = 1,
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };

    err = esp_lcd_new_panel_sh8601(s_io, &panel_config, &s_panel);

    if (err == ESP_OK)
    {
        err = esp_lcd_panel_reset(s_panel);
    }

    if (err == ESP_OK)
    {
        err = esp_lcd_panel_init(s_panel);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Panel init: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "SH8601 %dx%d ready (QSPI %d MHz)", BOARD_LCD_WIDTH, BOARD_LCD_HEIGHT, CONFIG_BOARD_LCD_PCLK_MHZ);

    return ESP_OK;
}

esp_err_t display_draw(int x1, int y1, int x2, int y2, const void *pixels)
{
    return esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2 + 1, y2 + 1, pixels);
}

/* The SH8601 only accepts windows starting on even and ending on odd pixels. */
void display_round_area(int *x1, int *y1, int *x2, int *y2)
{
    *x1 &= ~1;
    *y1 &= ~1;
    *x2 |= 1;
    *y2 |= 1;
}

esp_err_t display_set_brightness(uint8_t level)
{
    return send_command(CMD_BRIGHTNESS, &level, 1);
}

esp_err_t display_sleep(bool sleep)
{
    if (sleep == s_sleeping)
    {
        return ESP_OK;
    }

    esp_err_t err;

    if (sleep)
    {
        uint8_t zero = 0;
        send_command(CMD_BRIGHTNESS, &zero, 1);
        send_command(CMD_DISPLAY_OFF, NULL, 0);
        err = send_command(CMD_SLEEP_IN, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    else
    {
        err = send_command(CMD_SLEEP_OUT, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(120));   /* required by the controller before more commands */
        send_command(CMD_DISPLAY_ON, NULL, 0);
    }

    s_sleeping = sleep;

    return err;
}

uint16_t display_width(void)
{
    return BOARD_LCD_WIDTH;
}

uint16_t display_height(void)
{
    return BOARD_LCD_HEIGHT;
}
