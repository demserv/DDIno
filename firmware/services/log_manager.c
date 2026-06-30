#include "log_manager.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "log_mgr";

static const char *severity_str(log_severity_t s)
{
    switch (s) {
        case LOG_SEVERITY_DEBUG:    return "DBG";
        case LOG_SEVERITY_INFO:     return "INF";
        case LOG_SEVERITY_WARNING:  return "WRN";
        case LOG_SEVERITY_ERROR:    return "ERR";
        case LOG_SEVERITY_CRITICAL: return "CRI";
        default:                    return "???";
    }
}

esp_err_t log_manager_init(void)
{
    return storage_atomic_init();
}

esp_err_t log_manager_append(log_severity_t severity, const char *module, const char *fmt, ...)
{
    if (!module || !fmt) return ESP_ERR_INVALID_ARG;

    char buf[ATOMIC_ENTRY_MAX_LEN];
    char msg[ATOMIC_ENTRY_MAX_LEN - 32];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    snprintf(buf, sizeof(buf), "[%s][%s] %s", severity_str(severity), module, msg);

    uint32_t now = (uint32_t)(esp_log_timestamp());
    esp_err_t err = storage_atomic_append(now, buf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to append log: %s", buf);
    }
    return err;
}

uint32_t log_manager_count(void)
{
    return storage_atomic_available();
}

esp_err_t log_manager_get_recent(log_record_t *out, uint32_t count, uint32_t *actual)
{
    if (!out || !actual) return ESP_ERR_INVALID_ARG;

    uint32_t avail = storage_atomic_available();
    if (avail == 0) {
        *actual = 0;
        return ESP_OK;
    }

    uint32_t start_seq = 0;
    atomic_ring_entry_t ring_buf[64];
    uint32_t read = 0;
    uint32_t to_read = (count < 64) ? count : 64;

    esp_err_t err = storage_atomic_read(start_seq, ring_buf, to_read, &read);
    if (err != ESP_OK) return err;

    for (uint32_t i = 0; i < read && i < count; i++) {
        log_record_t *rec = &out[i];
        rec->timestamp_ms = ring_buf[i].timestamp_ms;

        char *bracket1 = strchr(ring_buf[i].data, '[');
        if (bracket1) {
            bracket1++;
            char *bracket2 = strchr(bracket1, ']');
            if (bracket2) {
                size_t sev_len = bracket2 - bracket1;
                if (sev_len < 4) {
                    if (strncmp(bracket1, "DBG", 3) == 0) rec->severity = LOG_SEVERITY_DEBUG;
                    else if (strncmp(bracket1, "INF", 3) == 0) rec->severity = LOG_SEVERITY_INFO;
                    else if (strncmp(bracket1, "WRN", 3) == 0) rec->severity = LOG_SEVERITY_WARNING;
                    else if (strncmp(bracket1, "ERR", 3) == 0) rec->severity = LOG_SEVERITY_ERROR;
                    else if (strncmp(bracket1, "CRI", 3) == 0) rec->severity = LOG_SEVERITY_CRITICAL;
                    else rec->severity = LOG_SEVERITY_INFO;
                }

                char *module_start = bracket2 + 2;
                char *module_end = strchr(module_start, ']');
                if (module_end) {
                    size_t mod_len = module_end - module_start;
                    if (mod_len >= sizeof(rec->module)) mod_len = sizeof(rec->module) - 1;
                    strncpy(rec->module, module_start, mod_len);
                    rec->module[mod_len] = '\0';

                    const char *msg_start = module_end + 2;
                    strncpy(rec->message, msg_start, sizeof(rec->message) - 1);
                    rec->message[sizeof(rec->message) - 1] = '\0';
                } else {
                    rec->module[0] = '\0';
                    strncpy(rec->message, ring_buf[i].data, sizeof(rec->message) - 1);
                }
            } else {
                rec->severity = LOG_SEVERITY_INFO;
                rec->module[0] = '\0';
                strncpy(rec->message, ring_buf[i].data, sizeof(rec->message) - 1);
            }
        } else {
            rec->severity = LOG_SEVERITY_INFO;
            rec->module[0] = '\0';
            strncpy(rec->message, ring_buf[i].data, sizeof(rec->message) - 1);
        }
    }

    *actual = read;
    return ESP_OK;
}

esp_err_t log_manager_clear(void)
{
    return storage_atomic_flush();
}

