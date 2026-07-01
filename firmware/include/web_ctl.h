// @requirement RF-WEB-001 a RF-WEB-008 Gerenciamento web estendido: exportação, SD, health check
#ifndef FIRMWARE_INCLUDE_WEB_CTL_H
#define FIRMWARE_INCLUDE_WEB_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t web_ctl_init(void);
void web_ctl_tick(void);
esp_err_t web_ctl_export_config(void);
esp_err_t web_ctl_import_config(void);
esp_err_t web_ctl_get_health_json(char *buf, size_t buf_size);
bool web_ctl_is_sd_available(void);
esp_err_t web_ctl_recover_creds(void);
esp_err_t web_ctl_handle_sd_full(void);

#ifdef __cplusplus
}
#endif

#endif
