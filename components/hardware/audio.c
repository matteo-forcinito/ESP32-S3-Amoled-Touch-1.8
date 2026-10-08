#include "hardware/audio.h"

#include "hardware/i2c_bus.h"

#include "board_config.h"

#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "audio";

#if BOARD_HAS_AUDIO

static i2s_chan_handle_t s_tx = NULL;
static const audio_codec_data_if_t *s_data_if = NULL;
static const audio_codec_ctrl_if_t *s_ctrl_if = NULL;
static const audio_codec_gpio_if_t *s_gpio_if = NULL;
static const audio_codec_if_t *s_codec_if = NULL;
static esp_codec_dev_handle_t s_codec = NULL;
static esp_pm_lock_handle_t s_pm_lock = NULL;
static bool s_lock_held = false;
static uint32_t s_rate = 0;
static int s_channels = 0;

static esp_err_t i2s_open(uint32_t rate, int channels)
{
    i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_config.auto_clear = true;     /* silence instead of old data on underrun */
    chan_config.dma_desc_num = 6;
    chan_config.dma_frame_num = 480;   /* ~60 ms of queue at 48 kHz */

    esp_err_t err = i2s_new_channel(&chan_config, &s_tx, NULL);

    if (err != ESP_OK)
    {
        return err;
    }

    i2s_std_config_t std_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        channels == 2 ? I2S_SLOT_MODE_STEREO : I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = BOARD_I2S_PIN_MCLK,
            .bclk = BOARD_I2S_PIN_BCLK,
            .ws = BOARD_I2S_PIN_WS,
            .dout = BOARD_I2S_PIN_DOUT,
            .din = I2S_GPIO_UNUSED,
        },
    };

    return i2s_channel_init_std_mode(s_tx, &std_config);
}

esp_err_t audio_start(uint32_t sample_rate, int channels)
{
    channels = channels == 2 ? 2 : 1;

    if (s_codec != NULL)
    {
        if (sample_rate == s_rate && channels == s_channels)
        {
            return ESP_OK;
        }

        audio_stop();   /* reopen with the new format */
    }

    if (s_pm_lock == NULL)
    {
        esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "audio", &s_pm_lock);
    }

    if (s_pm_lock != NULL && !s_lock_held)
    {
        esp_pm_lock_acquire(s_pm_lock);
        s_lock_held = true;
    }

    esp_err_t err = i2s_open(sample_rate, channels);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S: %s", esp_err_to_name(err));
        audio_stop();
        return err;
    }

    audio_codec_i2s_cfg_t i2s_config = {.port = I2S_NUM_0, .tx_handle = s_tx};
    s_data_if = audio_codec_new_i2s_data(&i2s_config);

    audio_codec_i2c_cfg_t i2c_config = {
        .port = I2C_NUM_0,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus_handle(),
    };
    s_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_config);
    s_gpio_if = audio_codec_new_gpio();

    if (s_data_if == NULL || s_ctrl_if == NULL || s_gpio_if == NULL)
    {
        audio_stop();
        return ESP_FAIL;
    }

    es8311_codec_cfg_t es8311_config = {
        .ctrl_if = s_ctrl_if,
        .gpio_if = s_gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BOARD_AUDIO_PIN_PA,
        .use_mclk = true,
        .hw_gain = {
            .pa_voltage = 5.0f,
            .codec_dac_voltage = 3.3f,
        },
    };
    s_codec_if = es8311_codec_new(&es8311_config);

    esp_codec_dev_cfg_t dev_config = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = s_codec_if,
        .data_if = s_data_if,
    };
    s_codec = (s_codec_if != NULL) ? esp_codec_dev_new(&dev_config) : NULL;

    if (s_codec == NULL)
    {
        ESP_LOGE(TAG, "ES8311 not found");
        audio_stop();
        return ESP_FAIL;
    }

    esp_codec_dev_sample_info_t format = {
        .bits_per_sample = 16,
        .channel = (uint8_t)channels,
        .sample_rate = sample_rate,
    };

    if (esp_codec_dev_open(s_codec, &format) != ESP_CODEC_DEV_OK)
    {
        ESP_LOGE(TAG, "Cannot open the codec");
        audio_stop();
        return ESP_FAIL;
    }

    s_rate = sample_rate;
    s_channels = channels;
    ESP_LOGI(TAG, "Output %lu Hz, %d ch", (unsigned long)sample_rate, channels);

    return ESP_OK;
}

esp_err_t audio_set_volume(int percent)
{
    if (s_codec == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    percent = percent < 0 ? 0 : (percent > 100 ? 100 : percent);

    return esp_codec_dev_set_out_vol(s_codec, percent) == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t audio_write(const int16_t *samples, size_t count)
{
    if (s_codec == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int ret = esp_codec_dev_write(s_codec, (void *)samples, (int)(count * sizeof(int16_t)));

    return ret == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}

void audio_stop(void)
{
    if (s_codec != NULL)
    {
        esp_codec_dev_close(s_codec);   /* also switches the amplifier off */
        esp_codec_dev_delete(s_codec);
        s_codec = NULL;
    }

    if (s_codec_if != NULL)
    {
        audio_codec_delete_codec_if(s_codec_if);
        s_codec_if = NULL;
    }

    if (s_ctrl_if != NULL)
    {
        audio_codec_delete_ctrl_if(s_ctrl_if);
        s_ctrl_if = NULL;
    }

    if (s_gpio_if != NULL)
    {
        audio_codec_delete_gpio_if(s_gpio_if);
        s_gpio_if = NULL;
    }

    if (s_data_if != NULL)
    {
        audio_codec_delete_data_if(s_data_if);
        s_data_if = NULL;
    }

    if (s_tx != NULL)
    {
        i2s_channel_disable(s_tx);   /* may already be disabled: error ignored */
        i2s_del_channel(s_tx);
        s_tx = NULL;
    }

    if (s_lock_held)
    {
        esp_pm_lock_release(s_pm_lock);
        s_lock_held = false;
    }

    s_rate = 0;
    s_channels = 0;
}

bool audio_is_running(void)
{
    return s_codec != NULL;
}

#else

esp_err_t audio_start(uint32_t sample_rate, int channels) { (void)sample_rate; (void)channels; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t audio_set_volume(int percent) { (void)percent; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t audio_write(const int16_t *samples, size_t count) { (void)samples; (void)count; return ESP_ERR_NOT_SUPPORTED; }
void audio_stop(void) { (void)TAG; }
bool audio_is_running(void) { return false; }

#endif
