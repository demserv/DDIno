// @requirement RF-PLUG-001 a RF-PLUG-014 Gerenciamento completo de plugues
// @requirement RF-FEED-001 Comportamento dos plugues em Feed Mode
#ifndef FIRMWARE_SERVICES_PLUG_MANAGER_H
#define FIRMWARE_SERVICES_PLUG_MANAGER_H

#include "esp_err.h"
#include "plug_model.h"
#include "system_types.h"
#include "services/plug_preset_catalog.h"

void plug_manager_init(void);
void plug_manager_tick(uint64_t now_s, system_state_t sys_state, bool feed_active);
plug_mode_t plug_manager_get_mode(plug_id_t id);
esp_err_t plug_manager_set_mode(plug_id_t id, plug_mode_t mode);
bool plug_manager_get_effective_state(plug_id_t id);
esp_err_t plug_manager_toggle(plug_id_t id, bool on);
esp_err_t plug_manager_toggle_ex(plug_id_t id, bool on, uint64_t now_ms);
esp_err_t plug_manager_relocate(plug_id_t src_id, plug_id_t dst_id);
bool plug_manager_is_relocated(plug_id_t id);
void plug_manager_set_plug_current(plug_id_t id, float current_a);
uint8_t plug_manager_active_count(void);
plug_model_t *plug_manager_get(plug_id_t id);
void plug_manager_apply_safe_off(void);
/* @requirement RF-PLUG-014 Bloqueio/desbloqueio persistente de um plugue (curto-circuito
 * ou sobrecorrente persistente). Plugue bloqueado é forçado OFF até reset manual. */
void plug_manager_set_blocked(plug_id_t id, bool blocked);
/* @requirement RF-FEED-001/RF-PLUG-006 Máscara (bit i = plug index i) dos plugues do
 * tipo BOMBA, usada pelo Feed Mode para desligar apenas as bombas. */
uint32_t plug_manager_get_pump_mask(void);
/* @requirement RF-PLUG-006 Máscara dos plugues atualmente energizados (estado real),
 * usada para capturar o snapshot do Feed Mode na borda de ativação. */
uint32_t plug_manager_get_on_mask(void);
void plug_manager_set_thermal_request(plug_id_t id, bool heater_on, bool cooler_on);
void plug_manager_set_ato_request(bool pump_on);
void plug_manager_set_restart_mask(uint16_t mask);

/* P03..P10: nome customizado ou preset da biblioteca (item 7). */
esp_err_t plug_manager_set_custom_name(plug_id_t id, const char *name);
esp_err_t plug_manager_apply_preset(plug_id_t id, plug_preset_id_t preset_id);

#endif
