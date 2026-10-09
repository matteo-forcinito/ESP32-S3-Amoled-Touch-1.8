#include "hardware/sdcard.h"

#include "board_config.h"

#include "esp_log.h"

static const char *TAG = "sdcard";

#if BOARD_HAS_SDCARD

#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sd_protocol_defs.h"
#include "sdmmc_cmd.h"

#include <stdio.h>
#include <stdlib.h>

#define FORMAT_UNIT  (32 * 1024)   /* cluster size for a new file system: fewer FAT writes */

static sdmmc_card_t *s_card = NULL;
static sdmmc_card_t *s_raw = NULL;
static esp_err_t s_last_err = ESP_ERR_NOT_FOUND;   /* why the last mount failed */

static sdmmc_slot_config_t slot_config(void)
{
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = BOARD_SD_PIN_CLK;
    slot.cmd = BOARD_SD_PIN_CMD;
    slot.d0 = BOARD_SD_PIN_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    return slot;
}

static esp_err_t try_mount(int freq_khz, bool format)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = freq_khz;

    sdmmc_slot_config_t slot = slot_config();

    esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = format,
        .max_files = 8,
        .allocation_unit_size = format ? FORMAT_UNIT : 16 * 1024,
    };

    return esp_vfs_fat_sdmmc_mount(SDCARD_MOUNT, &host, &slot, &mount, &s_card);
}

esp_err_t sdcard_mount(void)
{
    if (s_card != NULL)
    {
        return ESP_OK;
    }

    /* High speed first (faster app installs), then the safe default speed. */
    esp_err_t err = try_mount(SDMMC_FREQ_HIGHSPEED, false);

    if (err != ESP_OK && err != ESP_ERR_TIMEOUT)
    {
        err = try_mount(SDMMC_FREQ_DEFAULT, false);
    }

    s_last_err = err;

    if (err != ESP_OK)
    {
        s_card = NULL;
        ESP_LOGW(TAG, "Mount failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Mounted %s, %llu MB", s_card->cid.name,
             ((unsigned long long)s_card->csd.capacity * s_card->csd.sector_size) / (1024 * 1024));

    return ESP_OK;
}

void sdcard_unmount(void)
{
    if (s_card != NULL)
    {
        esp_vfs_fat_sdcard_unmount(SDCARD_MOUNT, s_card);
        s_card = NULL;
    }
}

bool sdcard_is_mounted(void)
{
    return s_card != NULL;
}

esp_err_t sdcard_open_raw(sdmmc_card_t **card)
{
    if (s_raw != NULL)
    {
        *card = s_raw;
        return ESP_OK;
    }

    sdcard_unmount();

    s_raw = calloc(1, sizeof(sdmmc_card_t));

    if (s_raw == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = slot_config();

    esp_err_t err = sdmmc_host_init();

    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE)
    {
        err = sdmmc_host_init_slot(host.slot, &slot);
    }

    if (err == ESP_OK)
    {
        err = sdmmc_card_init(&host, s_raw);
    }

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Card init for USB failed: %s", esp_err_to_name(err));
        sdmmc_host_deinit();
        free(s_raw);
        s_raw = NULL;
        return err;
    }

    *card = s_raw;

    return ESP_OK;
}

sdcard_state_t sdcard_state(void)
{
    if (s_raw != NULL)
    {
        return SDCARD_BUSY;
    }

    if (s_card != NULL)
    {
        return SDCARD_MOUNTED;
    }

    /* No card-detect pin here: ESP_FAIL means the card answered but has no FAT. */
    return s_last_err == ESP_FAIL ? SDCARD_UNREADABLE : SDCARD_ABSENT;
}

esp_err_t sdcard_info(sdcard_info_t *out)
{
    if (s_card == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    snprintf(out->name, sizeof(out->name), "%s", s_card->cid.name);
    out->size_mb = (uint32_t)(((uint64_t)s_card->csd.capacity * s_card->csd.sector_size) / (1024 * 1024));
    out->freq_khz = (uint32_t)s_card->real_freq_khz;
    out->high_capacity = (s_card->ocr & SD_OCR_SDHC_CAP) != 0;

    return ESP_OK;
}

esp_err_t sdcard_format(void)
{
    if (s_raw != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;

    if (s_card != NULL)
    {
        esp_vfs_fat_mount_config_t config = {
            .max_files = 8,
            .allocation_unit_size = FORMAT_UNIT,
        };
        err = esp_vfs_fat_sdcard_format_cfg(SDCARD_MOUNT, s_card, &config);
    }
    else
    {
        /* Unreadable card: mounting with "format if it fails" creates the file system. */
        err = try_mount(SDMMC_FREQ_DEFAULT, true);

        if (err != ESP_OK)
        {
            s_card = NULL;
        }
    }

    s_last_err = err;
    ESP_LOGI(TAG, "Format: %s", esp_err_to_name(err));

    return err;
}

void sdcard_space(uint32_t *total_mb, uint32_t *free_mb)
{
    uint64_t total = 0;
    uint64_t free_bytes = 0;

    if (s_card != NULL)
    {
        esp_vfs_fat_info(SDCARD_MOUNT, &total, &free_bytes);
    }

    *total_mb = (uint32_t)(total / (1024 * 1024));
    *free_mb = (uint32_t)(free_bytes / (1024 * 1024));
}

#else

esp_err_t sdcard_mount(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sdcard_open_raw(sdmmc_card_t **card) { (void)card; return ESP_ERR_NOT_SUPPORTED; }
void sdcard_unmount(void) { (void)TAG; }
bool sdcard_is_mounted(void) { return false; }
void sdcard_space(uint32_t *total_mb, uint32_t *free_mb) { *total_mb = 0; *free_mb = 0; }
sdcard_state_t sdcard_state(void) { return SDCARD_ABSENT; }
esp_err_t sdcard_info(sdcard_info_t *out) { (void)out; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sdcard_format(void) { return ESP_ERR_NOT_SUPPORTED; }

#endif
