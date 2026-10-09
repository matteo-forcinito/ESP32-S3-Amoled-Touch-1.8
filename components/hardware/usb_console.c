#include "hardware/usb_console.h"

#include "sdkconfig.h"

#if CONFIG_USJ_ENABLE_USB_SERIAL_JTAG

#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/usb_serial_jtag_ll.h"
#include "soc/rtc_cntl_struct.h"

static const char *TAG = "usb_console";

bool usb_console_connected(void)
{
    return usb_serial_jtag_is_connected();
}

/* The USB drive / USB keyboard (TinyUSB) took the port: not ours to touch. */
static bool port_used_by_otg(void)
{
    return RTCCNTL.usb_conf.sw_hw_usb_phy_sel && RTCCNTL.usb_conf.sw_usb_phy_sel;
}

void usb_console_reconnect(void)
{
    if (port_used_by_otg())
    {
        return;
    }

    /* Give a PC that is already enumerating the port the time to finish. */
    vTaskDelay(pdMS_TO_TICKS(100));

    if (usb_serial_jtag_is_connected() || port_used_by_otg())
    {
        return;
    }

    /* Drop the D+ pull-up for a moment: for the PC the cable was re-plugged. */
    const usb_serial_jtag_pull_override_vals_t off = {
        .dp_pu = false,
        .dm_pu = false,
        .dp_pd = false,
        .dm_pd = false,
    };

    usb_serial_jtag_ll_phy_enable_pull_override(&off);
    vTaskDelay(pdMS_TO_TICKS(20));
    usb_serial_jtag_ll_phy_disable_pull_override();

    ESP_LOGI(TAG, "USB port re-announced to the PC");
}

#else

bool usb_console_connected(void) { return false; }
void usb_console_reconnect(void) {}

#endif
