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

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "driver_acs712.h"
#include "driver_relay.h"
#include "relay_abstraction.h"
#include "global_state.h"
#include "hardware_config.h"
#include "services/alert_manager.h"
#include "services/audit_log.h"
#include "services/config_manager.h"
#include "services/plug_preset_catalog.h"
#include "safety_controller.h"
#include "alm_ids.h"

extern global_state_t g_gs;

static const char *TAG = "plug_manager";

static plug_model_t s_plugs[PLUG_COUNT_TOTAL];
static bool s_thermal_heater = false;
static bool s_thermal_cooler = false;
/* @requirement RF-ATO-002 Pedido de bomba ATO vem exclusivamente da ato_fsm. */
static bool s_ato_pump_request = false;
static uint64_t s_last_toggle_ms[PLUG_COUNT_TOTAL] = {0};
static uint8_t s_bypass_sample_count[PLUG_COUNT_TOTAL] = {0};
/* @requirement RF-FEED-001 Estado congelado dos plugues não-BOMBA no instante da
 * entrada em Feed Mode (os demais mantêm exatamente o estado de então). */
static bool s_feed_hold_active = false;
static bool s_feed_hold_state[PLUG_COUNT_TOTAL] = {0};

static bool plug_startup_delay_active(plug_id_t id);

/* @requirement RF-PLUG-001 Rota única de acionamento: o plug_manager é o serviço
 * autorizado de atuação (a jusante do command_validator). Toda comutação passa
 * pela camada de abstração; para relés críticos a confirmação é armada aqui pois
 * o comando já foi validado upstream. */
static esp_err_t plug_actuate(plug_id_t id, bool on, bool is_manual)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return ESP_ERR_INVALID_ARG;
    /* @requirement decisao normativa 2026-06-30: P01/P02 aguardam 5 s no boot. */
    if (on && plug_startup_delay_active(id)) {
        return ESP_ERR_INVALID_STATE;
    }
    relay_id_t rid = (relay_id_t)(id - 1);
    /* @requirement RF-PLUG-011 Dupla confirmação só em desligamento MANUAL de crítico;
     * automação (tick/FSM) pode desligar sem confirm. Validator/API já validaram
     * confirm=true antes de chegar aqui via toggle_ex. */
    if (is_manual && !on && s_plugs[id - 1].is_critical) {
        relay_abstraction_arm_critical_confirm(rid);
    }
    return relay_abstraction_set(rid, on);
}

/* @requirement RF-PLUG-002 / SRS v3.11 §5.10 e AF.3: P01=AQUECEDOR (GPIO direto),
 * P02=COOLER (GPIO direto), P03..P10 gerais via MCP23017. A bomba de ATO é um
 * plugue geral (default em P03). Mapa alinhado a relay_abstraction e command_validator. */
static const char *s_default_names[PLUG_COUNT_TOTAL] = {
    "Aquecedor", "Cooler", "Bomba ATO", "Filtro",
    "Aireador", "Luz", "P07", "P08", "P09", "P10"
};

static plug_type_t s_default_types[PLUG_COUNT_TOTAL] = {
    PLUG_TYPE_AQUECEDOR, PLUG_TYPE_COOLER, PLUG_TYPE_BOMBA, PLUG_TYPE_FILTRO,
    PLUG_TYPE_AIREADOR, PLUG_TYPE_LUZ, PLUG_TYPE_OUTRO, PLUG_TYPE_OUTRO,
    PLUG_TYPE_OUTRO, PLUG_TYPE_OUTRO
};

static bool s_in_safe_off = false;
static uint16_t s_restart_energized_mask = 0;
static uint64_t s_boot_ms = 0;

static bool plug_startup_delay_active(plug_id_t id)
{
    if (id != PLUG_ID_P01 && id != PLUG_ID_P02) return false;
    uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    return (now_ms - s_boot_ms) < (uint64_t)HW_RELAY_P01_P02_STARTUP_DELAY_MS;
}

void plug_manager_reload_limits(void)
{
    const plug_limits_storage_t *pl = config_get_plug_limits();
    if (!pl) {
        return;
    }
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        s_plugs[i].min_on_time_s = pl->min_on_time_s;
        s_plugs[i].min_off_time_s = pl->min_off_time_s;
    }
    ESP_LOGI(TAG, "Plug limits NVS: min_on=%lus min_off=%lus",
             (unsigned long)pl->min_on_time_s, (unsigned long)pl->min_off_time_s);
}

void plug_manager_init(void)
{
    s_boot_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
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
    plug_manager_reload_limits();
    memset(s_bypass_sample_count, 0, sizeof(s_bypass_sample_count));
    s_in_safe_off = false;
    ESP_LOGI(TAG, "Plug manager initialized, %d plugs", PLUG_COUNT_TOTAL);
}

void plug_manager_tick(uint64_t now_s, system_state_t sys_state, bool feed_active)
{
    s_in_safe_off = (sys_state >= SYSTEM_STATE_SAFE_OFF);

    /* @requirement RF-FEED-001 Captura do estado instantâneo dos plugues não-BOMBA
     * na borda de entrada do Feed Mode; o snapshot congela os demais plugues. */
    if (feed_active && !s_feed_hold_active && !s_in_safe_off) {
        for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
            s_feed_hold_state[i] = relay_get(s_plugs[i].id);
        }
        s_feed_hold_active = true;
    } else if (!feed_active && s_feed_hold_active) {
        s_feed_hold_active = false;
    }

    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        plug_model_t *p = &s_plugs[i];
        bool target_on = false;

        if (g_gs.monitor_only_mode) {
            /* @requirement RF-INSTALL-MONITOR-001 Em modo somente-monitor os relés
             * permanecem OFF: nenhuma automação local pode energizar cargas. */
            target_on = false;
            p->monitor_only_blocked = true;
            p->blocked_by_safe_state = false;
        } else if (s_in_safe_off) {
            p->monitor_only_blocked = false;
            if (s_restart_energized_mask & (1U << i)) {
                target_on = true;
                p->blocked_by_safe_state = false;
            } else {
                target_on = false;
                p->blocked_by_safe_state = true;
            }
        } else if (feed_active) {
            /* @requirement RF-FEED-001 Só os plugues BOMBA desligam; AQUECEDOR/COOLER
             * não participam; os demais mantêm o estado do instante de ativação. */
            p->monitor_only_blocked = false;
            p->blocked_by_safe_state = false;
            if (p->type == PLUG_TYPE_BOMBA) {
                target_on = false;
            } else if (p->type == PLUG_TYPE_AQUECEDOR) {
                target_on = s_thermal_heater;
            } else if (p->type == PLUG_TYPE_COOLER) {
                target_on = s_thermal_cooler;
            } else {
                target_on = s_feed_hold_state[i];
            }
        } else {
            p->monitor_only_blocked = false;
            p->blocked_by_safe_state = false;

            switch (p->mode) {
                case PLUG_MODE_AUTO: {
                    if (p->type == PLUG_TYPE_AQUECEDOR) {
                        target_on = s_thermal_heater;
                    } else if (p->type == PLUG_TYPE_COOLER) {
                        target_on = s_thermal_cooler;
                    } else if (p->type == PLUG_TYPE_BOMBA) {
                        /* RF-ATO-002: bomba ATO segue exclusivamente a ato_fsm. */
                        target_on = s_ato_pump_request;
                    } else if (p->type == PLUG_TYPE_FILTRO
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

        /* @requirement RF-PLUG-014/RB-PLUG-008 Plugue bloqueado (curto/sobrecorrente
         * persistente) permanece desenergizado até reset manual, independente do modo. */
        if (p->blocked) {
            target_on = false;
        }

        bool startup_wait = (plug_startup_delay_active(p->id) &&
                             (p->type == PLUG_TYPE_AQUECEDOR || p->type == PLUG_TYPE_COOLER) &&
                             (s_thermal_heater || s_thermal_cooler));

        if (target_on && plug_startup_delay_active(p->id)) {
            target_on = false;
        }

        bool current = relay_get(p->id);
        uint64_t now_ms = now_s * MS_PER_SEC;

        if (target_on != current) {
            uint64_t elapsed = now_ms - s_last_toggle_ms[i];
            if (target_on) {
                if (elapsed < (uint64_t)p->min_off_time_s * MS_PER_SEC) {
                    target_on = current;
                }
            } else if (elapsed < (uint64_t)p->min_on_time_s * MS_PER_SEC) {
                target_on = current;
            }
        }

        if (target_on != current) {
            plug_actuate(p->id, target_on, false);
            s_last_toggle_ms[i] = now_ms;
        }

        if (startup_wait) {
            p->effective_state = PLUG_EFFECTIVE_STATE_WAITING;
        } else {
            p->effective_state = target_on ? PLUG_EFFECTIVE_STATE_ON : PLUG_EFFECTIVE_STATE_OFF;
        }
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

uint32_t plug_manager_get_pump_mask(void)
{
    uint32_t mask = 0;
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        if (s_plugs[i].type == PLUG_TYPE_BOMBA) {
            mask |= (1U << i);
        }
    }
    return mask;
}

uint32_t plug_manager_get_on_mask(void)
{
    uint32_t mask = 0;
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        if (relay_get(s_plugs[i].id)) {
            mask |= (1U << i);
        }
    }
    return mask;
}

void plug_manager_set_blocked(plug_id_t id, bool blocked)
{
    plug_model_t *p = plug_manager_get(id);
    if (!p) return;
    if (p->blocked != blocked) {
        ESP_LOGW(TAG, "Plug %d blocked=%d (RF-PLUG-014)", (int)id, (int)blocked);
    }
    p->blocked = blocked;
    if (blocked) {
        plug_actuate(id, false, false);
    }
}

void plug_manager_set_thermal_request(plug_id_t id, bool heater_on, bool cooler_on)
{
    (void)id;
    /* @requirement RF-THERMAL-009 Exclusão mútua: em nenhuma hipótese a camada de
     * atuação recebe aquecedor e cooler simultaneamente ligados. Conflito → ambos
     * OFF (a thermal_fsm já força SAFE_OFF/ALM-060 na origem). */
    if (heater_on && cooler_on) {
        ESP_LOGE(TAG, "RF-THERMAL-009: heater+cooler simultaneos negados; ambos OFF");
        s_thermal_heater = false;
        s_thermal_cooler = false;
        return;
    }
    s_thermal_heater = heater_on;
    s_thermal_cooler = cooler_on;
}

void plug_manager_set_ato_request(bool pump_on)
{
    s_ato_pump_request = pump_on;
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
    /* @requirement RF-PLUG-008 Caminho manual/API usa o tempo real para que o
     * mínimo ON/OFF seja efetivamente aplicado (antes era now_ms=0 = desabilitado). */
    uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    return plug_manager_toggle_ex(id, on, now_ms);
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

    esp_err_t act = plug_actuate(id, on, true);
    if (act != ESP_OK) return act;
    if (now_ms > 0) {
        s_last_toggle_ms[id - 1] = now_ms;
    }
    p->effective_state = on ? PLUG_EFFECTIVE_STATE_ON : PLUG_EFFECTIVE_STATE_OFF;

    /* @requirement RF-PLUG-011 Desligamento manual confirmado de plugue crítico
     * (P01/P02) → ALM-065 + DEGRADED (mesma política de api_rest plug_set_handler). */
    if (p->is_critical && !on) {
        uint64_t now_s = now_ms > 0 ? (now_ms / MS_PER_SEC) : (uint64_t)(esp_timer_get_time() / 1000000ULL);
        alert_manager_raise_full(ALM_065, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_SYSTEM,
            "Desligamento manual de plugue critico", (float)id,
            "Reative o plugue critico ou confirme realocacao", (uint16_t)id,
            true, true, now_s);
        global_state_enter_degraded(&g_gs, "plug_manager_critical_off");
        audit_log_event(AUDIT_COMMAND, "Desligamento manual de plugue critico (P01/P02)");
    }

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
    relay_abstraction_all_off();
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

esp_err_t plug_manager_set_custom_name(plug_id_t id, const char *name)
{
    if (id < PLUG_ID_P03 || id > PLUG_ID_P10) return ESP_ERR_INVALID_ARG;
    if (!name || !name[0]) return ESP_ERR_INVALID_ARG;
    plug_model_t *p = &s_plugs[id - 1];
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    return ESP_OK;
}

esp_err_t plug_manager_apply_preset(plug_id_t id, plug_preset_id_t preset_id)
{
    if (id < PLUG_ID_P03 || id > PLUG_ID_P10) return ESP_ERR_INVALID_ARG;
    const plug_preset_t *preset = plug_preset_find_by_id(preset_id);
    if (!preset) return ESP_ERR_NOT_FOUND;
    plug_model_t *p = &s_plugs[id - 1];
    strncpy(p->name, preset->name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->type = preset->type;
    p->is_critical = (preset->type == PLUG_TYPE_AQUECEDOR || preset->type == PLUG_TYPE_COOLER);
    return ESP_OK;
}

// RF-PLUG-004: Set plug current from ACS712 reading
void plug_manager_set_plug_current(plug_id_t id, float current_a)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return;
    s_plugs[id - 1].current_a = current_a;
}

