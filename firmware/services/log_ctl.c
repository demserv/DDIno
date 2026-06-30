// @requirement RF-LOG-001 a RF-LOG-020 Sistema de logging estendido
#include "log_ctl.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#define LOG_CTL_MAX_ENTRIES 128
#define LOG_CTL_LINE_MAX 256

static const char *TAG = "log_ctl";
static log_ctl_level_t s_level = LOG_LEVEL_INFO;
static uint8_t s_dest_mask = 0x01;
static char s_buffer[LOG_CTL_MAX_ENTRIES][LOG_CTL_LINE_MAX];
static int s_head = 0;
static int s_count = 0;

static const char *level_str(log_ctl_level_t lvl)
{
    switch (lvl) {
        case LOG_LEVEL_ERROR:   return "E";
        case LOG_LEVEL_WARN:    return "W";
        case LOG_LEVEL_INFO:    return "I";
        case LOG_LEVEL_DEBUG:   return "D";
        case LOG_LEVEL_VERBOSE: return "V";
        default:                return "?";
    }
}

esp_err_t log_ctl_init(void)
{
    s_level = LOG_LEVEL_INFO;
    s_dest_mask = LOG_DEST_SERIAL;
    s_head = 0;
    s_count = 0;
    ESP_LOGI(TAG, "Log control initialized");
    return ESP_OK;
}

esp_err_t log_ctl_set_level(log_ctl_level_t level)
{
    s_level = level;
    return ESP_OK;
}

log_ctl_level_t log_ctl_get_level(void) { return s_level; }

esp_err_t log_ctl_set_destination(uint8_t dest_mask)
{
    s_dest_mask = dest_mask;
    return ESP_OK;
}

uint8_t log_ctl_get_destination(void) { return s_dest_mask; }

esp_err_t log_ctl_write(log_ctl_level_t level, const char *tag, const char *msg)
{
    if (level > s_level) return ESP_OK;
    if (s_dest_mask & LOG_DEST_SERIAL) {
        esp_log_write((esp_log_level_t)level, tag, "%s (%s) %s\n", level_str(level), tag, msg);
    }
    if (s_count < LOG_CTL_MAX_ENTRIES) {
        int n = snprintf(s_buffer[s_head], LOG_CTL_LINE_MAX, "%s (%s) %s", level_str(level), tag, msg);
        if (n >= LOG_CTL_LINE_MAX) s_buffer[s_head][LOG_CTL_LINE_MAX - 1] = '\0';
        s_head = (s_head + 1) % LOG_CTL_MAX_ENTRIES;
        s_count++;
    }
    return ESP_OK;
}

esp_err_t log_ctl_write_fmt(log_ctl_level_t level, const char *tag, const char *fmt, ...)
{
    char buf[LOG_CTL_LINE_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return log_ctl_write(level, tag, buf);
}

int log_ctl_get_entry_count(void) { return (s_count < LOG_CTL_MAX_ENTRIES) ? s_count : LOG_CTL_MAX_ENTRIES; }

esp_err_t log_ctl_export(char *buf, size_t buf_size, int max_lines)
{
    if (!buf || buf_size == 0) return ESP_ERR_INVALID_ARG;
    int total = log_ctl_get_entry_count();
    int lines = (max_lines > 0 && max_lines < total) ? max_lines : total;
    int start = (s_count <= LOG_CTL_MAX_ENTRIES) ? 0 : (s_head - lines + LOG_CTL_MAX_ENTRIES) % LOG_CTL_MAX_ENTRIES;
    int pos = 0;
    for (int i = 0; i < lines && pos < (int)buf_size - 1; i++) {
        int idx = (start + i) % LOG_CTL_MAX_ENTRIES;
        pos += snprintf(buf + pos, buf_size - pos, "%s\n", s_buffer[idx]);
        if (pos >= (int)buf_size - 1) break;
    }
    buf[buf_size - 1] = '\0';
    return ESP_OK;
}

esp_err_t log_ctl_clear(void) { s_head = 0; s_count = 0; return ESP_OK; }
esp_err_t log_ctl_flush(void) { return ESP_OK; }

