#ifndef CORE_SYS_H
#define CORE_SYS_H

/*
 * Small system helpers used everywhere.
 *
 * sys_task_create(): every background job (radio, web, installer, scans)
 * starts a task, and a task stack must live in internal RAM. If it cannot be
 * created the feature silently does nothing - the worst kind of bug on a
 * watch. This wrapper logs why (with the heap state) and returns false so
 * the caller can show an error.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SYS_CORE_ANY tskNO_AFFINITY

bool sys_task_create(TaskFunction_t fn, const char *name, uint32_t stack, void *arg, UBaseType_t priority,
                     BaseType_t core);

/* "internal 123 KB free (largest 60 KB, min 80 KB), PSRAM 7.5 MB free" in the log. */
void sys_heap_log(const char *where);

/* Free internal RAM in bytes and the largest block. */
size_t sys_internal_free(void);
size_t sys_internal_largest(void);
size_t sys_internal_min_free(void);

#endif
