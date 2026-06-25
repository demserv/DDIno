#ifndef FIRMWARE_SERVICES_PLUG_MANAGER_H
#define FIRMWARE_SERVICES_PLUG_MANAGER_H

#include "esp_err.h"
#include "plug_model.h"
#include "system_types.h"

void plug_manager_init(void);
void plug_manager_tick(uint64_t now_s, system_state_t sys_state, bool feed_active);
plug_mode_t plug_manager_get_mode(plug_id_t id);
esp_err_t plug_manager_set_mode(plug_id_t id, plug_mode_t mode);
bool plug_manager_get_effective_state(plug_id_t id);
esp_err_t plug_manager_toggle(plug_id_t id, bool on);
uint8_t plug_manager_active_count(void);
plug_model_t *plug_manager_get(plug_id_t id);
void plug_manager_apply_safe_off(void);
void plug_manager_set_thermal_request(plug_id_t id, bool heater_on, bool cooler_on);

#endif
