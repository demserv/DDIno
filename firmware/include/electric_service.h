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
/* @requirement RF-DADOS-001 Fonte única: publicada pelo laço de controle a partir
 * da electric_fsm autoritativa + leituras PZEM/ACS712 do laço. */
void electric_service_publish(float power_w, float current_a, float voltage_v, bool overload);

#ifdef __cplusplus
}
#endif

#endif
