#include "hardware/sdcard.h"

#include "board_config.h"

#include "esp_log.h"

static const char *TAG = "sdcard";

#if BOARD_HAS_SDCARD

#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static sdmmc_card_t *s_card = NULL;

static esp_err_t try_mount(int freq_khz)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = freq_khz;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = BOARD_SD_PIN_CLK;
    slot.cmd = BOARD_SD_PIN_CMD;
    slot.d0 = BOARD_SD_PIN_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
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
    esp_err_t err = try_mount(SDMMC_FREQ_HIGHSPEED);

    if (err != ESP_OK && err != ESP_ERR_TIMEOUT)
    {
        err = try_mount(SDMMC_FREQ_DEFAULT);
    }

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
void sdcard_unmount(void) { (void)TAG; }
bool sdcard_is_mounted(void) { return false; }
void sdcard_space(uint32_t *total_mb, uint32_t *free_mb) { *total_mb = 0; *free_mb = 0; }

#endif
