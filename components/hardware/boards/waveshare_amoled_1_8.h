#ifndef WAVESHARE_AMOLED_1_8_H
#define WAVESHARE_AMOLED_1_8_H

/*
 * Waveshare ESP32-S3-Touch-AMOLED-1.8
 *
 *   ESP32-S3R8, 16 MB flash, 8 MB octal PSRAM
 *   1.8" AMOLED 368x448, SH8601 controller on QSPI
 *   FT3168 capacitive touch, AXP2101 PMU (battery, PWR key),
 *   PCF85063 RTC, QMI8658 IMU, ES8311 codec + speaker amp, microSD (SDMMC 1-bit)
 *
 * Same pins as the Arduino launcher (pin_config.h from Waveshare).
 */

#define BOARD_NAME              "Waveshare AMOLED 1.8"

/* ---- display (SH8601, QSPI) */
#define BOARD_LCD_WIDTH         368
#define BOARD_LCD_HEIGHT        448
#define BOARD_LCD_ROUND         0
#define BOARD_LCD_PIN_CS        12
#define BOARD_LCD_PIN_SCLK      11
#define BOARD_LCD_PIN_D0        4
#define BOARD_LCD_PIN_D1        5
#define BOARD_LCD_PIN_D2        6
#define BOARD_LCD_PIN_D3        7
#define BOARD_LCD_PIN_RST       -1      /* reset comes from the IO expander */

/* ---- shared I2C bus: touch, PMU, RTC, IMU, codec, IO expander */
#define BOARD_I2C_PIN_SDA       15
#define BOARD_I2C_PIN_SCL       14

/* ---- touch (FT3168) */
#define BOARD_HAS_TOUCH         1
#define BOARD_TOUCH_ADDR        0x38
#define BOARD_TOUCH_PIN_INT     21

/* ---- TCA9554 IO expander: outputs 0..2 drive the display/touch resets */
#define BOARD_HAS_IO_EXPANDER   1
#define BOARD_IO_EXPANDER_ADDR  0x20
#define BOARD_IO_EXPANDER_RESET_MASK 0x07

/* ---- power (AXP2101): battery gauge, charger, PWR key */
#define BOARD_HAS_PMU           1
#define BOARD_PMU_ADDR          0x34

/* ---- RTC (PCF85063) */
#define BOARD_HAS_RTC           1
#define BOARD_RTC_ADDR          0x51

/* ---- audio (ES8311 + NS4150 amplifier) */
#define BOARD_HAS_AUDIO         1
#define BOARD_I2S_PIN_MCLK      16
#define BOARD_I2S_PIN_BCLK      9
#define BOARD_I2S_PIN_WS        45
#define BOARD_I2S_PIN_DOUT      8
#define BOARD_I2S_PIN_DIN       10
#define BOARD_AUDIO_PIN_PA      46

/* ---- microSD (SDMMC, 1-bit) */
#define BOARD_HAS_SDCARD        1
#define BOARD_SD_PIN_CLK        2
#define BOARD_SD_PIN_CMD        1
#define BOARD_SD_PIN_D0         3

/* ---- buttons: BOOT on a GPIO, PWR on the PMU */
#define BOARD_PIN_BUTTON_BOOT   0

#endif
