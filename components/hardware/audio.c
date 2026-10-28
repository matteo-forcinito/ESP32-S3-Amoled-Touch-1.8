#include "hardware/audio.h"
#include "hardware/microphone.h"

#include "hardware/i2c_bus.h"

#include "board_config.h"
#include "audio_supply.h"

#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

static const char *TAG = "audio";
/* Requested DMA queue (0 = board default: 6 x 480 frames) and the open one. */
static int s_req_frames, s_req_num, s_open_frames, s_open_num;
#define AUDIO_QUEUE_FRAMES (s_req_frames ? s_req_frames : 480)
#define AUDIO_QUEUE_NUM (s_req_num ? s_req_num : 6)

#if BOARD_HAS_AUDIO

static i2s_chan_handle_t s_tx = NULL;
static i2s_chan_handle_t s_rx = NULL;
static SemaphoreHandle_t s_mutex = NULL;
static bool s_capture = false;
static TaskHandle_t s_owner = NULL;
static uint32_t s_overflows = 0;
static void stop_unlocked(void);

/* Lazy creation is safe even when the sound task and recorder start together. */
static bool audio_lock(void)
{
    SemaphoreHandle_t mutex = __atomic_load_n(&s_mutex, __ATOMIC_ACQUIRE);
    if (mutex == NULL)
    {
        SemaphoreHandle_t created = xSemaphoreCreateMutex();
        if (created == NULL)
        {
            return false;
        }
        SemaphoreHandle_t expected = NULL;
        if (!__atomic_compare_exchange_n(&s_mutex, &expected, created, false,
                                        __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
        {
            vSemaphoreDelete(created);
        }
        mutex = __atomic_load_n(&s_mutex, __ATOMIC_ACQUIRE);
    }
    return xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) == pdTRUE;
}

static bool on_rx_overflow(i2s_chan_handle_t channel, i2s_event_data_t *event, void *arg)
{
    (void)channel;
    (void)event;
    (void)arg;
    __atomic_add_fetch(&s_overflows, 1, __ATOMIC_RELAXED);
    return false;
}
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
    chan_config.dma_desc_num = s_capture ? 6 : (s_open_num = AUDIO_QUEUE_NUM);
    chan_config.dma_frame_num = s_capture ? 256 : (s_open_frames = AUDIO_QUEUE_FRAMES);   /* default ~60 ms at 48 kHz */

    esp_err_t err = i2s_new_channel(&chan_config, s_capture ? NULL : &s_tx, s_capture ? &s_rx : NULL);

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
            .dout = s_capture ? I2S_GPIO_UNUSED : BOARD_I2S_PIN_DOUT,
            .din = s_capture ? BOARD_I2S_PIN_DIN : I2S_GPIO_UNUSED,
        },
    };

    err = i2s_channel_init_std_mode(s_capture ? s_rx : s_tx, &std_config);
    if (err == ESP_OK && s_capture)
    {
        const i2s_event_callbacks_t callbacks = {.on_recv_q_ovf = on_rx_overflow};
        err = i2s_channel_register_event_callback(s_rx, &callbacks, NULL);
    }
    return err;
}

static esp_err_t start_unlocked(uint32_t sample_rate, int channels, bool capture)
{
#if CONFIG_WATCH_MONKEY_TEST
    /* The monkey test may start a countdown by mistake: the speaker stays off, always. */
    (void)sample_rate;
    (void)channels;
    return ESP_ERR_NOT_SUPPORTED;
#endif

    if (sample_rate < 8000 || sample_rate > 48000 || (channels != 1 && channels != 2) ||
        (capture && (channels != 1 || sample_rate != 16000)))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_codec != NULL && (s_capture != capture || s_owner != xTaskGetCurrentTaskHandle()))
    {
        return ESP_ERR_INVALID_STATE;
    }
    channels = channels == 2 ? 2 : 1;

    if (s_codec != NULL)
    {
        if (sample_rate == s_rate && channels == s_channels &&
            (capture || (AUDIO_QUEUE_FRAMES == s_open_frames && AUDIO_QUEUE_NUM == s_open_num)))
        {
            return ESP_OK;
        }

        stop_unlocked();   /* reopen with the new format */
    }

    s_capture = capture;
    __atomic_store_n(&s_overflows, 0, __ATOMIC_RELAXED);

    if (s_pm_lock == NULL)
    {
        esp_err_t lock_err = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "audio", &s_pm_lock);
        if (lock_err != ESP_OK)
        {
            stop_unlocked();
            return lock_err;
        }
    }

    if (s_pm_lock != NULL && !s_lock_held)
    {
        esp_err_t lock_err = esp_pm_lock_acquire(s_pm_lock);
        if (lock_err != ESP_OK)
        {
            stop_unlocked();
            return lock_err;
        }
        s_lock_held = true;
    }

    esp_err_t err = audio_supply_acquire();
    if (err != ESP_OK)
    {
        stop_unlocked();
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    err = i2s_open(sample_rate, capture ? 2 : channels);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S: %s", esp_err_to_name(err));
        stop_unlocked();
        return err;
    }

    audio_codec_i2s_cfg_t i2s_config = {.port = I2S_NUM_0, .tx_handle = s_tx, .rx_handle = s_rx};
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
        stop_unlocked();
        return ESP_FAIL;
    }

    es8311_codec_cfg_t es8311_config = {
        .ctrl_if = s_ctrl_if,
        .gpio_if = s_gpio_if,
        .codec_mode = capture ? ESP_CODEC_DEV_WORK_MODE_ADC : ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = capture ? -1 : BOARD_AUDIO_PIN_PA,
        .use_mclk = true,
        .no_dac_ref = capture, /* Capture microphone PCM on both serial slots. */
        .hw_gain = {
            .pa_voltage = 5.0f,
            .codec_dac_voltage = 3.3f,
        },
    };
    s_codec_if = es8311_codec_new(&es8311_config);

    esp_codec_dev_cfg_t dev_config = {
        .dev_type = capture ? ESP_CODEC_DEV_TYPE_IN : ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = s_codec_if,
        .data_if = s_data_if,
    };
    s_codec = (s_codec_if != NULL) ? esp_codec_dev_new(&dev_config) : NULL;

    if (s_codec == NULL)
    {
        ESP_LOGE(TAG, "ES8311 not found");
        stop_unlocked();
        return ESP_FAIL;
    }

    esp_codec_dev_sample_info_t format = {
        .bits_per_sample = 16,
        .channel = (uint8_t)(capture ? 2 : channels),
        .sample_rate = sample_rate,
    };

    if (esp_codec_dev_open(s_codec, &format) != ESP_CODEC_DEV_OK)
    {
        ESP_LOGE(TAG, "Cannot open the codec");
        stop_unlocked();
        return ESP_FAIL;
    }

    if (capture && esp_codec_dev_set_in_gain(s_codec, 24.0f) != ESP_CODEC_DEV_OK)
    {
        stop_unlocked();
        return ESP_FAIL;
    }
#if CONFIG_WATCH_DEV_CONSOLE
    if (capture)
    {
        const int registers[] = {0x01, 0x02, 0x03, 0x0A, 0x0E, 0x13, 0x14, 0x16, 0x17, 0x18, 0x44};
        for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); i++)
        {
            int value = 0;
            int result = esp_codec_dev_read_reg(s_codec, registers[i], &value);
            ESP_LOGI(TAG, "ADC register %02x = %02x (%d)", registers[i], value, result);
        }
    }
#endif
    s_owner = xTaskGetCurrentTaskHandle();
    s_rate = sample_rate;
    s_channels = channels;
    ESP_LOGI(TAG, "%s %lu Hz, %d ch", capture ? "Input" : "Output", (unsigned long)sample_rate, channels);

    return ESP_OK;
}

static esp_err_t volume_unlocked(int percent)
{
    if (s_codec == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    percent = percent < 0 ? 0 : (percent > 100 ? 100 : percent);

    return esp_codec_dev_set_out_vol(s_codec, percent) == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}

static esp_err_t write_unlocked(const int16_t *samples, size_t count)
{
    if (s_codec == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int ret = esp_codec_dev_write(s_codec, (void *)samples, (int)(count * sizeof(int16_t)));

    return ret == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}

static void stop_unlocked(void)
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

    if (s_rx != NULL)
    {
        i2s_channel_disable(s_rx);
        i2s_del_channel(s_rx);
        s_rx = NULL;
    }
    audio_supply_release();
    s_capture = false;
    s_owner = NULL;
    s_rate = 0;
    s_channels = 0;
}

static esp_err_t audio_start_open(uint32_t rate, int channels)
{
    if (!audio_lock())
    {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = start_unlocked(rate, channels, false);
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t audio_set_volume(int percent)
{
    if (!audio_lock())
    {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = s_capture || s_owner != xTaskGetCurrentTaskHandle() ? ESP_ERR_INVALID_STATE : volume_unlocked(percent);
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t audio_write(const int16_t *samples, size_t count)
{
    if (samples == NULL || count == 0 || count > 8192)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!audio_lock())
    {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = s_capture || s_owner != xTaskGetCurrentTaskHandle() ? ESP_ERR_INVALID_STATE : write_unlocked(samples, count);
    xSemaphoreGive(s_mutex);
    return err;
}

void audio_stop(void)
{
    if (audio_lock())
    {
        if (!s_capture && s_owner == xTaskGetCurrentTaskHandle())
        {
            stop_unlocked();
        }
        xSemaphoreGive(s_mutex);
    }
}

bool audio_is_running(void)
{
    if (!audio_lock())
    {
        return true;
    }
    bool active = s_codec != NULL;
    xSemaphoreGive(s_mutex);
    return active;
}

bool microphone_supported(void)
{
#if CONFIG_WATCH_VOICE_MEMOS
    return true;
#else
    return false;
#endif
}

esp_err_t microphone_start(uint32_t rate, int channels)
{
    if (!microphone_supported())
    {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (!audio_lock())
    {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = s_codec != NULL ? ESP_ERR_INVALID_STATE : start_unlocked(rate, channels, true);
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t microphone_read(int16_t *samples, size_t capacity, size_t *count)
{
    if (count == NULL || samples == NULL || capacity < 2 || capacity > 512)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *count = 0;
    if (!audio_lock())
    {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (s_capture && s_rx != NULL && s_owner == xTaskGetCurrentTaskHandle())
    {
        size_t bytes = 0;
        err = i2s_channel_read(s_rx, samples, capacity * sizeof(int16_t), &bytes, 60);
        /* ES8311 emits two serial slots. With no_dac_ref both carry ADC
         * data; receive complete frames, then compact the left slot to mono.
         * The public API and saved WAV stay mono without an extra buffer. */
        if (bytes % (2 * sizeof(int16_t)) != 0)
        {
            err = ESP_ERR_INVALID_RESPONSE;
        }
        *count = bytes / (2 * sizeof(int16_t));
        for (size_t i = 0; i < *count; i++)
        {
            samples[i] = samples[i * 2];
        }
        if (__atomic_load_n(&s_overflows, __ATOMIC_RELAXED) != 0)
        {
            err = ESP_ERR_INVALID_RESPONSE; /* Never silently save a discontinuous memo. */
        }
    }
    xSemaphoreGive(s_mutex);
    return err;
}

void microphone_stop(void)
{
    if (audio_lock())
    {
        if (s_capture && s_owner == xTaskGetCurrentTaskHandle())
        {
            stop_unlocked();
        }
        xSemaphoreGive(s_mutex);
    }
}

/* audio_start() keeps the board's default queue; audio_start_ex() lets a caller
 * (Solco) choose it. A different queue reopens the output. */
esp_err_t audio_start_ex(uint32_t sample_rate, int channels, int frames_per_desc, int desc_num)
{
    if (frames_per_desc < 0 || (frames_per_desc && frames_per_desc < 8) || frames_per_desc > 1023 ||
        desc_num < 0 || desc_num == 1 || desc_num > 16)
    {
        return ESP_ERR_INVALID_ARG;
    }
    s_req_frames = frames_per_desc;
    s_req_num = desc_num;
    return audio_start_open(sample_rate, channels);
}

esp_err_t audio_start(uint32_t sample_rate, int channels)
{
    return audio_start_ex(sample_rate, channels, 0, 0);
}

int audio_queue_frames(void)
{
    return s_open_frames * s_open_num;
}

#else

esp_err_t audio_start(uint32_t sample_rate, int channels) { (void)sample_rate; (void)channels; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t audio_start_ex(uint32_t sample_rate, int channels, int frames_per_desc, int desc_num) { (void)sample_rate; (void)channels; (void)frames_per_desc; (void)desc_num; return ESP_ERR_NOT_SUPPORTED; }
int audio_queue_frames(void) { return 0; }
esp_err_t audio_set_volume(int percent) { (void)percent; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t audio_write(const int16_t *samples, size_t count) { (void)samples; (void)count; return ESP_ERR_NOT_SUPPORTED; }
void audio_stop(void) { (void)TAG; }
bool audio_is_running(void) { return false; }

bool microphone_supported(void) { return false; }
esp_err_t microphone_start(uint32_t rate, int channels) { (void)rate; (void)channels; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t microphone_read(int16_t *samples, size_t capacity, size_t *count)
{ (void)samples; (void)capacity; if (count != NULL) { *count = 0; } return ESP_ERR_NOT_SUPPORTED; }
void microphone_stop(void) {}

#endif
