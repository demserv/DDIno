#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_timer.h"

#include "services/log_manager.h"

static const char *TAG = "log_handler";

static log_severity_t esp_to_log_severity(char esp_level)
{
    switch (esp_level) {
        case 'E': return LOG_SEVERITY_ERROR;
        case 'W': return LOG_SEVERITY_WARNING;
        case 'I': return LOG_SEVERITY_INFO;
        case 'D': return LOG_SEVERITY_DEBUG;
        default:  return LOG_SEVERITY_INFO;
    }
}

#define LOG_HANDLER_BUF_SIZE 192

esp_err_t log_handler_write(const char *module, log_severity_t severity, const char *fmt, ...)
{
    char buf[LOG_HANDLER_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n < 0) return ESP_FAIL;
    return log_manager_append(severity, module, "%s", buf);
}

esp_err_t log_handler_write_esp(const char *module, char esp_level, const char *fmt, ...)
{
    char buf[LOG_HANDLER_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n < 0) return ESP_FAIL;
    log_severity_t sev = esp_to_log_severity(esp_level);
    return log_manager_append(sev, module, "%s", buf);
}

