#include "hardware/pmu.h"

#include "hardware/i2c_bus.h"

#include "board_config.h"

#include "esp_log.h"

static const char *TAG = "pmu";

#if BOARD_HAS_PMU

/* AXP2101 registers (see the XPowersLib register map). */
#define REG_STATUS1         0x00   /* bit5 VBUS good, bit3 battery present */
#define REG_STATUS2         0x01   /* bits 6:5 = 01 charging, 10 discharging */
#define REG_COMMON_CONFIG   0x10   /* bit0 = power off */
#define REG_GAUGE_CTRL      0x18   /* bit3 = fuel gauge on */
#define REG_ADC_ENABLE      0x30   /* bit0 battery voltage, bit2 VBUS, bit4 die temp */
#define REG_VBAT_H          0x34
#define REG_IRQ_ENABLE_2    0x41
#define REG_IRQ_STATUS_1    0x48
#define REG_IRQ_STATUS_2    0x49
#define REG_IRQ_STATUS_3    0x4A
#define REG_BATT_DETECT     0x68   /* bit0 = battery detection on */
#define REG_BATT_PERCENT    0xA4

#define IRQ2_PKEY_LONG      (1u << 2)
#define IRQ2_PKEY_SHORT     (1u << 3)

static i2c_master_dev_handle_t s_dev = NULL;

static uint8_t read_reg(uint8_t reg)
{
    uint8_t value = 0;

    if (s_dev != NULL)
    {
        i2c_bus_read_byte(s_dev, reg, &value);
    }

    return value;
}

static void set_bits(uint8_t reg, uint8_t mask)
{
    i2c_bus_write_byte(s_dev, reg, read_reg(reg) | mask);
}

esp_err_t pmu_init(void)
{
    if (!i2c_bus_probe(BOARD_PMU_ADDR))
    {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = i2c_bus_add(BOARD_PMU_ADDR, &s_dev);

    if (err != ESP_OK)
    {
        return err;
    }

    /* Measure the battery (fuel gauge + voltage ADC) and detect it. */
    set_bits(REG_BATT_DETECT, 0x01);
    set_bits(REG_GAUGE_CTRL, 0x08);
    set_bits(REG_ADC_ENABLE, 0x01 | 0x04);

    /* Report PWR key presses; clear whatever happened before we started. */
    set_bits(REG_IRQ_ENABLE_2, IRQ2_PKEY_SHORT | IRQ2_PKEY_LONG);
    i2c_bus_write_byte(s_dev, REG_IRQ_STATUS_1, 0xFF);
    i2c_bus_write_byte(s_dev, REG_IRQ_STATUS_2, 0xFF);
    i2c_bus_write_byte(s_dev, REG_IRQ_STATUS_3, 0xFF);

    ESP_LOGI(TAG, "AXP2101: battery %d%% %d mV%s%s", pmu_battery_percent(), pmu_battery_mv(),
             pmu_is_charging() ? ", charging" : "", pmu_usb_present() ? ", USB" : "");

    return ESP_OK;
}

bool pmu_present(void)
{
    return s_dev != NULL;
}

bool pmu_battery_present(void)
{
    return (read_reg(REG_STATUS1) & 0x08) != 0;
}

int pmu_battery_percent(void)
{
    if (s_dev == NULL || !pmu_battery_present())
    {
        return -1;
    }

    int percent = read_reg(REG_BATT_PERCENT);

    return percent > 100 ? 100 : percent;
}

int pmu_battery_mv(void)
{
    uint8_t data[2] = {0};

    if (s_dev == NULL || i2c_bus_read(s_dev, REG_VBAT_H, data, 2) != ESP_OK)
    {
        return 0;
    }

    return ((data[0] & 0x3F) << 8) | data[1];
}

int pmu_battery_mv_fresh(void)
{
    return pmu_battery_mv();   /* the AXP2101 register is always live */
}

bool pmu_is_charging(void)
{
    return ((read_reg(REG_STATUS2) >> 5) & 0x03) == 0x01;
}

bool pmu_usb_present(void)
{
    return (read_reg(REG_STATUS1) & 0x20) != 0;
}

pmu_key_t pmu_poll_key(void)
{
    if (s_dev == NULL)
    {
        return PMU_KEY_NONE;
    }

    uint8_t status = read_reg(REG_IRQ_STATUS_2);
    uint8_t keys = status & (IRQ2_PKEY_SHORT | IRQ2_PKEY_LONG);

    if (keys == 0)
    {
        return PMU_KEY_NONE;
    }

    i2c_bus_write_byte(s_dev, REG_IRQ_STATUS_2, keys);   /* write 1 to clear */

    return (keys & IRQ2_PKEY_LONG) ? PMU_KEY_LONG : PMU_KEY_SHORT;
}

void pmu_power_off(void)
{
    if (s_dev != NULL)
    {
        ESP_LOGW(TAG, "Power off");
        set_bits(REG_COMMON_CONFIG, 0x01);
    }
}

#else   /* boards without a PMU */

esp_err_t pmu_init(void) { return ESP_ERR_NOT_SUPPORTED; }
bool pmu_present(void) { return false; }
int pmu_battery_percent(void) { return -1; }
int pmu_battery_mv(void) { return 0; }
int pmu_battery_mv_fresh(void) { return 0; }
bool pmu_battery_present(void) { return false; }
bool pmu_is_charging(void) { return false; }
bool pmu_usb_present(void) { return true; }
pmu_key_t pmu_poll_key(void) { return PMU_KEY_NONE; }
void pmu_power_off(void) { ESP_LOGW(TAG, "No PMU: cannot power off"); }

#endif
