#include "hardware/knob.h"

/* This board has no rotary knob. */
esp_err_t knob_init(knob_cb_t callback)
{
    (void)callback;
    return ESP_ERR_NOT_SUPPORTED;
}
