// @requirement RF-PLUG-001 Modos de operacao por plugue
// @requirement RF-PLUG-002 Tipo e criticidade do plugue
// @requirement RF-PLUG-003 Protecao de corrente por plugue
// @requirement RF-PLUG-004 Bypass detection
// @requirement RF-PLUG-007 Estados visuais
// @requirement RF-PLUG-008 Tempo minimo ON/OFF
// @requirement RF-PLUG-012 Relocation AQUECEDOR/COOLER
// @requirement RF-PLUG-013 max_energy_wh_day monitoring
// @requirement RF-FEED-001 Comportamento dos plugues em Feed Mode
#include "services/plug_manager.h"
#include "services/alert_manager.h"
#include "services/config_manager.h"
#include "services/audit_log.h"
#include "driver_relay.h"
#include "driver_acs712.h"
#include "global_state.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

extern global_state_t g_gs;

static const char *TAG = "plug_manager";

static plug_model_t s_plugs[PLUG_COUNT_TOTAL];
static bool s_thermal_heater = false;
static bool s_thermal_cooler = false;
static uint64_t s_last_toggle_ms[PLUG_COUNT_TOTAL] = {0};
static uint8_t s_bypass_sample_count[PLUG_COUNT_TOTAL] = {0};

static const char *s_default_names[PLUG_COUNT_TOTAL] = {
    "Bomba ATO", "Aquecedor", "Cooler", "Filtro",
    "Aireador", "Luz", "P07", "P08", "P09", "P10"
};

static plug_type_t s_default_types[PLUG_COUNT_TOTAL] = {
    PLUG_TYPE_BOMBA, PLUG_TYPE_AQUECEDOR, PLUG_TYPE_COOLER, PLUG_TYPE_FILTRO,
    PLUG_TYPE_AIREADOR, PLUG_TYPE_LUZ, PLUG_TYPE_OUTRO, PLUG_TYPE_OUTRO,
    PLUG_TYPE_OUTRO, PLUG_TYPE_OUTRO
};

static bool s_in_safe_off = false;
static uint16_t s_restart_energized_mask = 0;

void plug_manager_init(void)
{
    memset(s_plugs, 0, sizeof(s_plugs));
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        plug_model_t *p = &s_plugs[i];
        p->id = (plug_id_t)(i + 1);
        strncpy(p->name, s_default_names[i], sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
        p->type = s_default_types[i];
        p->mode = PLUG_MODE_AUTO;
        p->command_allowed = true;
        p->blocked_by_safe_state = false;
        p->monitor_only_blocked = false;
        p->effective_state = PLUG_EFFECTIVE_STATE_OFF;
        p->visual_state = PLUG_VISUAL_STATE_OFF_NORMAL;
        p->is_critical = (p->type == PLUG_TYPE_AQUECEDOR || p->type == PLUG_TYPE_COOLER);
        p->current_limit_a = 10.0f;
        p->min_on_time_s = 60;
        p->min_off_time_s = 30;
        p->timer_on_s = 0;
        p->timer_off_s = 0;
        p->delay_seconds = 0;
        p->overcurrent_count = 0;
        p->energy_wh_today = 0;
        p->energy_wh_24h = 0;
        p->energy_wh_total = 0;
        p->fator_curto = 3.0f;
        p->tempo_deteccao_curto_ms = 500;
    }
    memset(s_bypass_sample_count, 0, sizeof(s_bypass_sample_count));
    s_in_safe_off = false;
    ESP_LOGI(TAG, "Plug manager initialized, %d plugs", PLUG_COUNT_TOTAL);
}

void plug_manager_tick(uint64_t now_s, system_state_t sys_state, bool feed_active)
{
    s_in_safe_off = (sys_state >= SYSTEM_STATE_SAFE_OFF);

    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        plug_model_t *p = &s_plugs[i];
        bool target_on = false;

        if (s_in_safe_off) {
            if (s_restart_energized_mask & (1U << i)) {
                target_on = true;
                p->blocked_by_safe_state = false;
            } else {
                target_on = false;
                p->blocked_by_safe_state = true;
            }
        } else if (feed_active && i < 4) {
            target_on = false;
        } else {
            p->blocked_by_safe_state = false;

            switch (p->mode) {
                case PLUG_MODE_AUTO: {
                    if (p->type == PLUG_TYPE_AQUECEDOR) {
                        target_on = s_thermal_heater;
                    } else if (p->type == PLUG_TYPE_COOLER) {
                        target_on = s_thermal_cooler;
                    } else if (p->type == PLUG_TYPE_FILTRO || p->type == PLUG_TYPE_BOMBA
                               || p->type == PLUG_TYPE_AIREADOR) {
                        target_on = true;
                    } else {
                        target_on = relay_get(p->id);
                    }
                    break;
                }
                case PLUG_MODE_MANUAL:
                    target_on = relay_get(p->id);
                    break;

                case PLUG_MODE_TIMER: {
                    uint32_t day_seconds = (uint32_t)(now_s % 86400ULL);
                    if (p->timer_on_s < p->timer_off_s) {
                        target_on = (day_seconds >= p->timer_on_s && day_seconds < p->timer_off_s);
                    } else {
                        target_on = (day_seconds >= p->timer_on_s || day_seconds < p->timer_off_s);
                    }
                    break;
                }
                case PLUG_MODE_DELAY: {
                    bool requested = relay_get(p->id);
                    uint64_t now_ms = now_s * MS_PER_SEC;
                    uint64_t elapsed = now_ms - s_last_toggle_ms[i];
                    if (elapsed < (uint64_t)p->delay_seconds * MS_PER_SEC) {
                        target_on = !requested;
                    } else {
                        target_on = requested;
                    }
                    break;
                }
                default:
                    target_on = relay_get(p->id);
                    break;
            }
        }

        bool current = relay_get(p->id);
        if (target_on != current) {
            relay_set(p->id, target_on);
            s_last_toggle_ms[i] = now_s * MS_PER_SEC;
        }

        p->effective_state = target_on ? PLUG_EFFECTIVE_STATE_ON : PLUG_EFFECTIVE_STATE_OFF;
        p->command_allowed = !s_in_safe_off;

        // RF-PLUG-004: Bypass detection
        if (p->effective_state == PLUG_EFFECTIVE_STATE_OFF) {
            if (p->current_a > p->current_limit_a * 0.1f) {
                s_bypass_sample_count[i]++;
                if (s_bypass_sample_count[i] >= 3 && !p->bypass_detected) {
                    p->bypass_detected = true;
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Bypass detected in %s", p->name);
                    alert_manager_raise_full((int16_t)ALM_054, ALERT_SEVERITY_HIGH,
                        ALERT_CATEGORY_PROCESS, msg, p->current_a,
                        "Check relay and wiring", (uint16_t)p->id,
                        true, true, now_s * MS_PER_SEC);
                }
            } else {
                s_bypass_sample_count[i] = 0;
            }
        } else {
            s_bypass_sample_count[i] = 0;
        }

        // RF-PLUG-013: max_energy_wh_day monitoring
        if (p->max_energy_wh_day > 0 && p->energy_wh_today > p->max_energy_wh_day) {
            if (!alert_manager_is_active((int16_t)ALM_057)) {
                char msg[64];
                snprintf(msg, sizeof(msg), "Daily energy limit exceeded for %s", p->name);
                alert_manager_raise_full((int16_t)ALM_057, ALERT_SEVERITY_WARNING,
                    ALERT_CATEGORY_PROCESS, msg, p->energy_wh_today,
                    "Check consumption", (uint16_t)p->id,
                    true, false, now_s * MS_PER_SEC);
            }
        }
    }
}

void plug_manager_set_thermal_request(plug_id_t id, bool heater_on, bool cooler_on)
{
    s_thermal_heater = heater_on;
    s_thermal_cooler = cooler_on;
}

plug_mode_t plug_manager_get_mode(plug_id_t id)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return PLUG_MODE_MANUAL;
    return s_plugs[id - 1].mode;
}

esp_err_t plug_manager_set_mode(plug_id_t id, plug_mode_t mode)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return ESP_ERR_INVALID_ARG;
    if (mode > PLUG_MODE_OVERRIDE) return ESP_ERR_INVALID_ARG;
    s_plugs[id - 1].mode = mode;
    return ESP_OK;
}

bool plug_manager_get_effective_state(plug_id_t id)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return false;
    return relay_get(id);
}

esp_err_t plug_manager_toggle(plug_id_t id, bool on)
{
    return plug_manager_toggle_ex(id, on, 0);
}

esp_err_t plug_manager_toggle_ex(plug_id_t id, bool on, uint64_t now_ms)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return ESP_ERR_INVALID_ARG;
    if (g_gs.monitor_only_mode) return ESP_ERR_INVALID_STATE;
    if (s_in_safe_off) return ESP_ERR_INVALID_STATE;
    if (s_plugs[id - 1].monitor_only_blocked) return ESP_ERR_INVALID_STATE;

    plug_model_t *p = &s_plugs[id - 1];

    if (now_ms > 0) {
        uint64_t elapsed = now_ms - s_last_toggle_ms[id - 1];
        if (on) {
            if (elapsed < (uint64_t)p->min_off_time_s * MS_PER_SEC)
                return ESP_ERR_INVALID_STATE;
        } else {
            if (elapsed < (uint64_t)p->min_on_time_s * MS_PER_SEC)
                return ESP_ERR_INVALID_STATE;
        }
    }

    relay_set(id, on);
    if (now_ms > 0) {
        s_last_toggle_ms[id - 1] = now_ms;
    }
    p->effective_state = on ? PLUG_EFFECTIVE_STATE_ON : PLUG_EFFECTIVE_STATE_OFF;
    return ESP_OK;
}

uint8_t plug_manager_active_count(void)
{
    uint8_t count = 0;
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        if (relay_get(s_plugs[i].id)) count++;
    }
    return count;
}

plug_model_t *plug_manager_get(plug_id_t id)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return NULL;
    return &s_plugs[id - 1];
}

void plug_manager_apply_safe_off(void)
{
    s_in_safe_off = true;
    s_restart_energized_mask = 0;
    relay_all_off();
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        s_plugs[i].effective_state = PLUG_EFFECTIVE_STATE_OFF;
        s_plugs[i].visual_state = PLUG_VISUAL_STATE_OFF_FAULT;
        s_plugs[i].blocked_by_safe_state = true;
    }
}

void plug_manager_set_restart_mask(uint16_t mask)
{
    s_restart_energized_mask = mask;
}

// RF-PLUG-012: Relocation AQUECEDOR/COOLER
esp_err_t plug_manager_relocate(plug_id_t src_id, plug_id_t dst_id)
{
    if (src_id != PLUG_ID_P01 && src_id != PLUG_ID_P02)
        return ESP_ERR_INVALID_ARG;
    if (dst_id < PLUG_ID_P03 || dst_id > PLUG_ID_P08)
        return ESP_ERR_INVALID_ARG;

    plug_model_t *src = &s_plugs[src_id - 1];
    plug_model_t *dst = &s_plugs[dst_id - 1];

    dst->type = src->type;
    src->type = PLUG_TYPE_OUTRO;

    src->role_override_source = (uint8_t)dst_id;
    dst->role_override_source = (uint8_t)src_id;

    src->is_critical = false;
    dst->is_critical = true;

    ESP_LOGI(TAG, "Relocated %s (P%02d) to P%02d", s_default_names[src_id - 1], src_id, dst_id);

    char audit_msg[64];
    snprintf(audit_msg, sizeof(audit_msg), "Relocated plug P%02d to P%02d", src_id, dst_id);
    audit_log_event(AUDIT_CONFIG_CHANGE, audit_msg);

    return ESP_OK;
}

bool plug_manager_is_relocated(plug_id_t id)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return false;
    return s_plugs[id - 1].role_override_source != 0;
}

// RF-PLUG-004: Set plug current from ACS712 reading
void plug_manager_set_plug_current(plug_id_t id, float current_a)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return;
    s_plugs[id - 1].current_a = current_a;
}
