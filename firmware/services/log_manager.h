#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "storage_atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_MANAGER_MAX_SEVERITY 16

typedef enum {
    LOG_SEVERITY_DEBUG = 0,
    LOG_SEVERITY_INFO,
    LOG_SEVERITY_WARNING,
    LOG_SEVERITY_ERROR,
    LOG_SEVERITY_CRITICAL
} log_severity_t;

typedef struct {
    log_severity_t  severity;
    uint32_t        timestamp_ms;
    char            module[24];
    char            message[ATOMIC_ENTRY_MAX_LEN - 48];
} log_record_t;

esp_err_t log_manager_init(void);
esp_err_t log_manager_append(log_severity_t severity, const char *module, const char *fmt, ...);
uint32_t  log_manager_count(void);
esp_err_t log_manager_get_recent(log_record_t *out, uint32_t count, uint32_t *actual);
esp_err_t log_manager_clear(void);

#ifdef __cplusplus
}
#endif
