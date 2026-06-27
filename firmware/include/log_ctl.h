// @requirement RF-LOG-001 a RF-LOG-020 Sistema de logging estendido: níveis, rotação, persistência
#ifndef FIRMWARE_INCLUDE_LOG_CTL_H
#define FIRMWARE_INCLUDE_LOG_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE
} log_ctl_level_t;

typedef enum {
    LOG_DEST_SERIAL = 0x01,
    LOG_DEST_SD     = 0x02,
    LOG_DEST_MEM    = 0x04,
    LOG_DEST_WEB    = 0x08
} log_ctl_dest_t;

esp_err_t log_ctl_init(void);
esp_err_t log_ctl_set_level(log_ctl_level_t level);
log_ctl_level_t log_ctl_get_level(void);
esp_err_t log_ctl_set_destination(uint8_t dest_mask);
uint8_t log_ctl_get_destination(void);
esp_err_t log_ctl_write(log_ctl_level_t level, const char *tag, const char *msg);
esp_err_t log_ctl_write_fmt(log_ctl_level_t level, const char *tag, const char *fmt, ...);
int log_ctl_get_entry_count(void);
esp_err_t log_ctl_export(char *buf, size_t buf_size, int max_lines);
esp_err_t log_ctl_clear(void);
esp_err_t log_ctl_flush(void);

#ifdef __cplusplus
}
#endif

#endif
