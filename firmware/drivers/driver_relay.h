// @requirement RF-PLUG-001 Controle de relés via GPIO/MCP23017
#ifndef DRIVER_RELAY_H
#define DRIVER_RELAY_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t relay_init_safe(void);
esp_err_t relay_all_off(void);
esp_err_t relay_set(uint8_t plug_id, bool on);
esp_err_t relay_set_p01(bool on);
esp_err_t relay_set_p02(bool on);
bool relay_mcp23017_ok(void);
esp_err_t relay_set_p03_p10_mask(uint8_t on_mask);
bool relay_get(uint8_t plug_id);

#endif
