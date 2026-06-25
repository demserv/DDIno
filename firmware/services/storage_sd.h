#ifndef FIRMWARE_SERVICES_STORAGE_SD_H
#define FIRMWARE_SERVICES_STORAGE_SD_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define SD_LOG_LINE_MAX_LEN  (256U)

typedef enum {
    SD_LOG_TYPE_EVENT = 0,
    SD_LOG_TYPE_ALERT,
    SD_LOG_TYPE_ENERGY,
    SD_LOG_TYPE_AUDIT
} sd_log_type_t;

esp_err_t storage_sd_init(void);
bool     storage_sd_is_mounted(void);
esp_err_t storage_sd_write_log(sd_log_type_t type, const char *line);
esp_err_t storage_sd_write_json_atomic(const char *filename, const char *json_content);
esp_err_t storage_sd_backup_config(void);
esp_err_t storage_sd_unmount(void);

#endif
