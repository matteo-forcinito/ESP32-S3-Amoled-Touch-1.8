#include "hardware/button.h"

#include "hardware/pmu.h"

#include "board_config.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define SAMPLE_MS      20
#define LONG_PRESS_MS  700
#define DEBOUNCE_MS    40

/*
 * The BOOT pin interrupt is level-triggered (that is what can wake the chip
 * from light sleep). The ISR disables it and starts a 20 ms sampling timer
 * that measures the press; when the button is released the interrupt is
 * enabled again. No task polls the button while it is not pressed.
 */

static button_cb_t s_callback = NULL;
static esp_timer_handle_t s_sampler = NULL;
static uint32_t s_held_ms = 0;
static bool s_long_sent = false;

static void sample(void *arg)
{
    (void)arg;

    bool pressed = gpio_get_level(BOARD_PIN_BUTTON_BOOT) == 0;

    if (pressed)
    {
        s_held_ms += SAMPLE_MS;

        if (!s_long_sent && s_held_ms >= LONG_PRESS_MS)
        {
            s_long_sent = true;

            if (s_callback != NULL)
            {
                s_callback(BUTTON_BOOT, BUTTON_LONG_PRESS);
            }
        }

        return;
    }

    esp_timer_stop(s_sampler);

    if (!s_long_sent && s_held_ms >= DEBOUNCE_MS && s_callback != NULL)
    {
        s_callback(BUTTON_BOOT, BUTTON_CLICK);
    }

    s_held_ms = 0;
    s_long_sent = false;
    gpio_intr_enable(BOARD_PIN_BUTTON_BOOT);
}

static void start_sampling(void *arg, uint32_t unused)
{
    (void)arg;
    (void)unused;

    esp_timer_start_periodic(s_sampler, SAMPLE_MS * 1000);
}

static void boot_isr(void *arg)
{
    (void)arg;

    BaseType_t woken = pdFALSE;

    gpio_intr_disable(BOARD_PIN_BUTTON_BOOT);

    /* Start the sampler from the FreeRTOS timer task, not from the interrupt. */
    xTimerPendFunctionCallFromISR(start_sampling, NULL, 0, &woken);
    portYIELD_FROM_ISR(woken);
}

esp_err_t button_init(button_cb_t callback)
{
    s_callback = callback;

    const esp_timer_create_args_t timer_args = {
        .callback = sample,
        .name = "button",
    };
    esp_err_t err = esp_timer_create(&timer_args, &s_sampler);

    if (err != ESP_OK)
    {
        return err;
    }

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BOARD_PIN_BUTTON_BOOT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_LOW_LEVEL,
    };
    gpio_config(&io);
    gpio_isr_handler_add(BOARD_PIN_BUTTON_BOOT, boot_isr, NULL);
    gpio_wakeup_enable(BOARD_PIN_BUTTON_BOOT, GPIO_INTR_LOW_LEVEL);

    return ESP_OK;
}

void button_poll_pmu(void)
{
    pmu_key_t key = pmu_poll_key();

    if (key != PMU_KEY_NONE && s_callback != NULL)
    {
        s_callback(BUTTON_PWR, key == PMU_KEY_LONG ? BUTTON_LONG_PRESS : BUTTON_CLICK);
    }
}
