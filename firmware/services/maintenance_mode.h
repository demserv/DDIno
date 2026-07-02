#ifndef FIRMWARE_SERVICES_MAINTENANCE_MODE_H
#define FIRMWARE_SERVICES_MAINTENANCE_MODE_H

// @requirement RF-UI-MENU-003 / RF-UI-MENU-003.1 Modo de manutenção com timeout
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

esp_err_t maintenance_mode_init(void);
bool      maintenance_mode_is_active(void);
esp_err_t maintenance_mode_activate(uint32_t duration_s);
esp_err_t maintenance_mode_deactivate(void);
void      maintenance_mode_tick(uint64_t now_s);
uint32_t  maintenance_mode_remaining_s(uint64_t now_s);
bool      maintenance_mode_is_expiring_soon(uint64_t now_s);

#define MAINT_PRE_EXPIRY_NOTIFY_S 300U

#endif
