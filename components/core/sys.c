#include "core/sys.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "sys";

size_t sys_internal_free(void)
{
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

size_t sys_internal_largest(void)
{
    return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

size_t sys_internal_min_free(void)
{
    return heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

void sys_heap_log(const char *where)
{
    ESP_LOGI(TAG, "[%s] internal %u KB free (largest %u KB, min %u KB), PSRAM %u KB free", where,
             (unsigned)(sys_internal_free() / 1024), (unsigned)(sys_internal_largest() / 1024),
             (unsigned)(sys_internal_min_free() / 1024),
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
}

bool sys_task_create(TaskFunction_t fn, const char *name, uint32_t stack, void *arg, UBaseType_t priority,
                     BaseType_t core)
{
    if (xTaskCreatePinnedToCore(fn, name, stack, arg, priority, NULL, core) == pdPASS)
    {
        return true;
    }

    ESP_LOGE(TAG, "Cannot start task \"%s\" (%u bytes of stack)", name, (unsigned)stack);
    sys_heap_log(name);

    return false;
}
