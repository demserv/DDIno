// @requirement RF-ATO-001..010 Gerenciamento ATO: nível, bomba, overflow
#ifndef FIRMWARE_INCLUDE_ATO_SERVICE_H
#define FIRMWARE_INCLUDE_ATO_SERVICE_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ato_service_init(void);
esp_err_t ato_service_get_level_adc(int32_t *out);
esp_err_t ato_service_is_pump_on(bool *out);
esp_err_t ato_service_is_overflow(bool *out);
esp_err_t ato_service_force_safe_off(const char *reason);
void ato_service_publish(bool pump_on, bool overflow);

#ifdef __cplusplus
}
#endif

#endif
