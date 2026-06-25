// @requirement RF-ENERGY-003 Acumuladores de energia
// @requirement RF-ENERGY-010 Log periódico SD
#ifndef FIRMWARE_SERVICES_CDN_ENERGY_H
#define FIRMWARE_SERVICES_CDN_ENERGY_H

#include "esp_err.h"
#include <stdint.h>

#define CDN_PLUG_COUNT 10

esp_err_t cdn_energy_init(void);
void cdn_energy_update(uint8_t plug_id, float current_a, float voltage_v, uint64_t delta_ms);
float cdn_energy_get_wh(uint8_t plug_id);
float cdn_energy_get_total_wh(void);
void cdn_energy_reset(uint8_t plug_id);
void cdn_energy_reset_all(void);
void cdn_energy_log_to_sd(void);

#endif
