#include "hardware/rtc.h"

#include "hardware/i2c_bus.h"

#include "board_config.h"

#include "esp_log.h"

static const char *TAG = "rtc";

#if BOARD_HAS_RTC

#define REG_CONTROL_2   0x01
#define REG_SECONDS     0x04
#define SECONDS_OS_FLAG 0x80   /* oscillator stopped: time not valid */

static i2c_master_dev_handle_t s_dev = NULL;

static uint8_t bcd_to_dec(uint8_t value)
{
    return (uint8_t)((value >> 4) * 10 + (value & 0x0F));
}

static uint8_t dec_to_bcd(int value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

/* Days since 1970-01-01 for a civil date (no time zone, no libc needed). */
static int64_t days_from_civil(int year, int month, int day)
{
    year -= month <= 2;
    const int64_t era = (year >= 0 ? year : year - 399) / 400;
    const int64_t yoe = year - era * 400;
    const int64_t doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

esp_err_t rtc_chip_init(void)
{
    if (!i2c_bus_probe(BOARD_RTC_ADDR))
    {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = i2c_bus_add(BOARD_RTC_ADDR, &s_dev);

    if (err == ESP_OK)
    {
        /* CLKOUT off, no interrupts: lowest power. */
        i2c_bus_write_byte(s_dev, REG_CONTROL_2, 0x07);
    }

    return err;
}

esp_err_t rtc_chip_get_time(time_t *out)
{
    uint8_t d[7];

    if (s_dev == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* One transaction: the chip freezes the registers while we read them. */
    esp_err_t err = i2c_bus_read(s_dev, REG_SECONDS, d, sizeof(d));

    if (err != ESP_OK)
    {
        return err;
    }

    if (d[0] & SECONDS_OS_FLAG)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int sec = bcd_to_dec(d[0] & 0x7F);
    int min = bcd_to_dec(d[1] & 0x7F);
    int hour = bcd_to_dec(d[2] & 0x3F);
    int day = bcd_to_dec(d[3] & 0x3F);
    int month = bcd_to_dec(d[5] & 0x1F);
    int year = 2000 + bcd_to_dec(d[6]);

    *out = (time_t)(days_from_civil(year, month, day) * 86400 + hour * 3600 + min * 60 + sec);

    return ESP_OK;
}

esp_err_t rtc_chip_set_time(time_t utc)
{
    struct tm t;

    if (s_dev == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    gmtime_r(&utc, &t);

    if (t.tm_year < 100 || t.tm_year > 199)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[7] = {
        dec_to_bcd(t.tm_sec),   /* bit 7 = 0 clears the "oscillator stopped" flag */
        dec_to_bcd(t.tm_min),
        dec_to_bcd(t.tm_hour),
        dec_to_bcd(t.tm_mday),
        (uint8_t)t.tm_wday,
        dec_to_bcd(t.tm_mon + 1),
        dec_to_bcd(t.tm_year - 100),
    };

    esp_err_t err = i2c_bus_write(s_dev, REG_SECONDS, data, sizeof(data));

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Set to %04d-%02d-%02d %02d:%02d:%02d UTC", t.tm_year + 1900, t.tm_mon + 1,
                 t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    }

    return err;
}

#else

esp_err_t rtc_chip_init(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t rtc_chip_get_time(time_t *out) { (void)out; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t rtc_chip_set_time(time_t utc) { (void)utc; (void)TAG; return ESP_ERR_NOT_SUPPORTED; }

#endif
