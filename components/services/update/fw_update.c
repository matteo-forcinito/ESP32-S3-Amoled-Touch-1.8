#include "services/fw_update.h"

#include "core/power.h"
#include "core/settings.h"
#include "core/state.h"
#include "core/sys.h"
#include "services/extapp.h"
#include "services/http_stream.h"
#include "services/wifi.h"

#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "fw_update";

/* Image header + first segment header + app description: enough to check the file. */
#define HEAD_SIZE      (sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
#define CHUNK          8192
#define RESTART_DELAY_US 2000000

static atomic_bool s_busy = false;
static fw_update_phase_t s_phase = FW_UPDATE_IDLE;
static char s_message[64] = "";
static char s_new_version[32] = "";

/* current install */
static const esp_partition_t *s_slot = NULL;
static esp_ota_handle_t s_ota = 0;
static bool s_ota_open = false;
static size_t s_total = 0;
static size_t s_written = 0;
static int s_percent = -1;
static uint8_t s_head[HEAD_SIZE];
static size_t s_head_len = 0;
static esp_timer_handle_t s_restart_timer = NULL;

/* ------------------------------------------------------------ status */

static void set_phase(fw_update_phase_t phase, const char *message)
{
    s_phase = phase;

    if (message != NULL)
    {
        snprintf(s_message, sizeof(s_message), "%s", message);
    }

    state_set(STATE_UPDATE, phase);
}

static void set_percent(int percent)
{
    if (percent != s_percent)
    {
        s_percent = percent;
        state_set(STATE_UPDATE_PERCENT, percent);   /* whole percents only: ~100 updates */
    }
}

const char *fw_update_running_version(void)
{
    return esp_app_get_description()->version;
}

fw_update_phase_t fw_update_phase(void)
{
    return s_phase;
}

int fw_update_percent(void)
{
    return s_percent < 0 ? 0 : s_percent;
}

const char *fw_update_message(void)
{
    return s_message;
}

const char *fw_update_new_version(void)
{
    return s_new_version;
}

/* ----------------------------------------------------------- install */

static void restart_job(void *arg)
{
    (void)arg;
    power_restart();
}

static void restart_timer_cb(void *arg)
{
    (void)arg;
    sys_post(restart_job, NULL);   /* never real work in an esp_timer callback */
}

static void release(void)
{
    power_cpu_boost(false);
    power_keep_screen_on(false);
    wifi_set_fast(false);
    atomic_store(&s_busy, false);
}

void fw_update_abort(const char *reason)
{
    if (!atomic_load(&s_busy) || s_phase != FW_UPDATE_WRITING)
    {
        return;
    }

    if (s_ota_open)
    {
        esp_ota_abort(s_ota);
        s_ota_open = false;
    }

    ESP_LOGE(TAG, "Update stopped: %s", reason);
    set_phase(FW_UPDATE_FAILED, reason);
    release();
}

esp_err_t fw_update_begin(size_t total_size)
{
    bool expected = false;

    if (!atomic_compare_exchange_strong(&s_busy, &expected, true))
    {
        return ESP_ERR_INVALID_STATE;   /* another update is running */
    }

    s_slot = esp_ota_get_next_update_partition(NULL);

    if (s_slot == NULL || total_size < HEAD_SIZE || total_size > s_slot->size)
    {
        set_phase(FW_UPDATE_FAILED, s_slot == NULL ? "Nessuno slot libero" : "Dimensione del file non valida");
        atomic_store(&s_busy, false);
        return ESP_ERR_INVALID_SIZE;
    }

    s_total = total_size;
    s_written = 0;
    s_head_len = 0;
    s_ota_open = false;
    s_percent = -1;

    power_cpu_boost(true);
    power_keep_screen_on(true);
    wifi_set_fast(true);

    set_percent(0);
    set_phase(FW_UPDATE_WRITING, "");
    ESP_LOGI(TAG, "Receiving %u KB into %s", (unsigned)(total_size / 1024), s_slot->label);

    return ESP_OK;
}

/* The first bytes are checked before the slot is erased. */
static esp_err_t check_head(void)
{
    const esp_image_header_t *image = (const esp_image_header_t *)s_head;
    const esp_app_desc_t *desc =
        (const esp_app_desc_t *)(s_head + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t));
    const esp_app_desc_t *running = esp_app_get_description();

    if (image->magic != ESP_IMAGE_HEADER_MAGIC || desc->magic_word != ESP_APP_DESC_MAGIC_WORD)
    {
        fw_update_abort("Non è un firmware (usa build/amoled_watch.bin)");
        return ESP_ERR_INVALID_ARG;
    }

    if (image->chip_id != CONFIG_IDF_FIRMWARE_CHIP_ID)
    {
        fw_update_abort("Firmware per un altro chip");
        return ESP_ERR_INVALID_VERSION;
    }

    if (strncmp(desc->project_name, running->project_name, sizeof(desc->project_name)) != 0)
    {
        fw_update_abort("Non è il firmware di sistema");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "New firmware %.32s (running %s)", desc->version, running->version);

    /* From here the app slot changes: no external app is cached in it any more. */
    extapp_forget_cache();

    esp_err_t err = esp_ota_begin(s_slot, OTA_WITH_SEQUENTIAL_WRITES, &s_ota);

    if (err != ESP_OK)
    {
        fw_update_abort("Flash non scrivibile");
        return err;
    }

    s_ota_open = true;
    err = esp_ota_write(s_ota, s_head, s_head_len);

    if (err != ESP_OK)
    {
        fw_update_abort("Errore di scrittura");
    }

    return err;
}

esp_err_t fw_update_write(const void *data, size_t length)
{
    if (!atomic_load(&s_busy) || s_phase != FW_UPDATE_WRITING)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const uint8_t *bytes = data;

    if (s_written + length > s_total)
    {
        fw_update_abort("Ricevuti più dati del previsto");
        return ESP_ERR_INVALID_SIZE;
    }

    s_written += length;

    if (!s_ota_open)
    {
        size_t take = HEAD_SIZE - s_head_len;
        take = take < length ? take : length;
        memcpy(s_head + s_head_len, bytes, take);
        s_head_len += take;
        bytes += take;
        length -= take;

        if (s_head_len < HEAD_SIZE)
        {
            return ESP_OK;
        }

        esp_err_t err = check_head();

        if (err != ESP_OK)
        {
            return err;
        }
    }

    if (length > 0)
    {
        esp_err_t err = esp_ota_write(s_ota, bytes, length);

        if (err != ESP_OK)
        {
            fw_update_abort("Errore di scrittura");
            return err;
        }
    }

    int percent = (int)((uint64_t)s_written * 100 / s_total);
    set_percent(percent >= 100 ? 99 : percent);   /* 100 only once verified */

    return ESP_OK;
}

esp_err_t fw_update_end(void)
{
    if (!atomic_load(&s_busy) || s_phase != FW_UPDATE_WRITING)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_ota_open || s_written != s_total)
    {
        fw_update_abort("Trasferimento incompleto");
        return ESP_ERR_INVALID_SIZE;
    }

    s_ota_open = false;
    esp_err_t err = esp_ota_end(s_ota);   /* SHA-256 of the whole image */

    if (err == ESP_OK)
    {
        err = esp_ota_set_boot_partition(s_slot);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Image rejected: %s", esp_err_to_name(err));
        set_phase(FW_UPDATE_FAILED, "Firmware danneggiato, riprova");
        release();
        return err;
    }

    set_percent(100);
    set_phase(FW_UPDATE_RESTARTING, "");
    ESP_LOGI(TAG, "Firmware installed in %s, restarting", s_slot->label);

    if (s_restart_timer == NULL)
    {
        const esp_timer_create_args_t args = {.callback = restart_timer_cb, .name = "fw_restart"};
        esp_timer_create(&args, &s_restart_timer);
    }

    /* a moment for the web page answer and the "restarting" screen */
    esp_timer_start_once(s_restart_timer, RESTART_DELAY_US);

    return ESP_OK;
}

/* ------------------------------------------------------------ online */

/* "1.10.2" > "1.9.0"? Plain text compare if it is not a version number. */
static bool is_newer(const char *candidate, const char *running)
{
    int a[3] = {0};
    int b[3] = {0};

    if (sscanf(candidate, "%d.%d.%d", &a[0], &a[1], &a[2]) >= 1 &&
        sscanf(running, "%d.%d.%d", &b[0], &b[1], &b[2]) >= 1)
    {
        for (int i = 0; i < 3; i++)
        {
            if (a[i] != b[i])
            {
                return a[i] > b[i];
            }
        }

        return false;
    }

    return strcmp(candidate, running) != 0;
}

static bool read_manifest(const char *manifest_url, char *bin_url, size_t bin_url_size)
{
    char *text = malloc(2048);

    if (text == NULL)
    {
        return false;
    }

    bool ok = false;

    if (http_stream_get_text(manifest_url, text, 2048, NULL, 0) == ESP_OK)
    {
        cJSON *json = cJSON_Parse(text);
        const char *version = cJSON_GetStringValue(cJSON_GetObjectItem(json, "version"));
        const char *url = cJSON_GetStringValue(cJSON_GetObjectItem(json, "url"));

        if (version != NULL && url != NULL)
        {
            snprintf(s_new_version, sizeof(s_new_version), "%s", version);
            snprintf(bin_url, bin_url_size, "%s", url);
            ok = true;
        }

        cJSON_Delete(json);
    }

    free(text);
    return ok;
}

static void download(const char *url)
{
    http_stream_t stream;
    uint8_t *buffer = malloc(CHUNK);

    if (buffer == NULL || http_stream_open(&stream, url, false) != ESP_OK)
    {
        free(buffer);
        set_phase(FW_UPDATE_FAILED, "Download non riuscito");
        return;
    }

    int64_t length = esp_http_client_get_content_length(stream.client);

    if (length <= 0 || fw_update_begin((size_t)length) != ESP_OK)
    {
        if (s_phase != FW_UPDATE_FAILED)
        {
            set_phase(FW_UPDATE_FAILED, "Dimensione del file sconosciuta");
        }

        http_stream_close(&stream);
        free(buffer);
        return;
    }

    esp_err_t err = ESP_OK;
    int got;

    while (err == ESP_OK && (got = http_stream_read(&stream, buffer, CHUNK)) > 0)
    {
        err = fw_update_write(buffer, (size_t)got);
    }

    http_stream_close(&stream);
    free(buffer);

    if (err == ESP_OK)
    {
        fw_update_end();   /* reports an incomplete download by itself */
    }
}

static void online_task(void *arg)
{
    bool install = (bool)(intptr_t)arg;
    char *bin_url = malloc(HTTP_STREAM_URL_MAX);
    const char *manifest_url = settings_get()->update_url;

    if (bin_url == NULL)
    {
        set_phase(FW_UPDATE_FAILED, "Memoria insufficiente");
    }
    else if (wifi_acquire(15000) != ESP_OK)
    {
        set_phase(FW_UPDATE_FAILED, "Wi-Fi non disponibile");
    }
    else
    {
        if (!read_manifest(manifest_url, bin_url, HTTP_STREAM_URL_MAX))
        {
            set_phase(FW_UPDATE_FAILED, "Manifest non leggibile");
        }
        else if (!is_newer(s_new_version, fw_update_running_version()))
        {
            set_phase(FW_UPDATE_UP_TO_DATE, "");
        }
        else if (!install)
        {
            set_phase(FW_UPDATE_AVAILABLE, "");
        }
        else
        {
            download(bin_url);
        }

        wifi_release();
    }

    free(bin_url);
    vTaskDelete(NULL);
}

void fw_update_check_online(bool install)
{
    if (atomic_load(&s_busy) || s_phase == FW_UPDATE_CHECKING)
    {
        return;
    }

    if (settings_get()->update_url[0] == '\0')
    {
        set_phase(FW_UPDATE_FAILED, "Indirizzo aggiornamenti non impostato");
        return;
    }

    set_phase(FW_UPDATE_CHECKING, "");

    /* TLS needs a deep stack */
    if (!sys_task_create(online_task, "fw_online", 8192, (void *)(intptr_t)install, 4, SYS_CORE_ANY))
    {
        set_phase(FW_UPDATE_FAILED, "Memoria insufficiente");
    }
}

/* ------------------------------------------------------------- boot */

void fw_update_confirm_boot(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    if (esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGI(TAG, "Firmware %s in %s confirmed", fw_update_running_version(), running->label);
    }

    esp_ota_mark_app_valid_cancel_rollback();
}
