#ifndef FIRMWARE_SERVICES_AUTH_RECOVERY_SD_H
#define FIRMWARE_SERVICES_AUTH_RECOVERY_SD_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t auth_recovery_sd_init(void);
esp_err_t auth_recovery_sd_process(bool maintenance_mode);

#endif
