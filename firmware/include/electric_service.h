// @requirement RF-ELECTRIC-BIVOLT-001 Gerenciamento elétrico: proteção bivolt, limites
#ifndef FIRMWARE_INCLUDE_ELECTRIC_SERVICE_H
#define FIRMWARE_INCLUDE_ELECTRIC_SERVICE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t electric_service_init(void);
esp_err_t electric_service_get_power_w(float *out);
esp_err_t electric_service_get_current_a(float *out);
esp_err_t electric_service_get_voltage_v(float *out);
esp_err_t electric_service_is_overload(bool *out);
esp_err_t electric_service_force_safe_off(const char *reason);

#ifdef __cplusplus
}
#endif

#endif
