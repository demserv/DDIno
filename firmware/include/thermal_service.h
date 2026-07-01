// @requirement RF-THERMAL-001..010 Gerenciamento térmico: setpoint, leitura, heater/cooler
#ifndef FIRMWARE_INCLUDE_THERMAL_SERVICE_H
#define FIRMWARE_INCLUDE_THERMAL_SERVICE_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t thermal_service_init(void);
esp_err_t thermal_service_get_setpoint(float *out_c);
esp_err_t thermal_service_set_setpoint(float c);
esp_err_t thermal_service_get_current(float *out_c);
esp_err_t thermal_service_is_heating(bool *out);
esp_err_t thermal_service_is_cooling(bool *out);
esp_err_t thermal_service_force_safe_off(const char *reason);
/* @requirement RF-DADOS-001 Fonte única: publicada pelo laço de controle a partir
 * da thermal_fsm autoritativa. */
void thermal_service_publish(float current_c, bool sample_valid, bool heating, bool cooling);

#ifdef __cplusplus
}
#endif

#endif
