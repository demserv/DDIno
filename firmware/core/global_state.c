// @requirement RF-GLOBAL-001 Definição de estados globais
// @requirement RF-GLOBAL-002 Transições com prioridade e rastreabilidade
// @requirement RNF-GLOBAL-ANTIFLAP-001 Anti-flap em transições
// @requirement RF-SAFEOFF-CAUSE-001 Rastreamento de causa SAFE_OFF
#include "global_state.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "audit_log.h"
#include "config_manager.h"
#include "relay_abstraction.h"
#include "event_bus.h"
#include "hardware_config.h"
#include "config_root.h"
#include "driver_acs712.h"
#include "safety_controller.h"
#include "services/plug_manager.h"

static const char *TAG = "global_state";

static global_state_t *s_gs = NULL;
static SemaphoreHandle_t s_mutex = NULL;

static uint64_t s_last_transition_ms;
static uint64_t s_flap_window_start_ms;
static uint32_t s_transition_count_in_window;

static void mutex_lock(void)
{
    if (s_mutex) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }
}

static void mutex_unlock(void)
{
    if (s_mutex) {
        xSemaphoreGive(s_mutex);
    }
}

static const char *state_to_str(system_state_t s)
{
    switch (s) {
        case SYSTEM_STATE_NORMAL:    return "NORMAL";
        case SYSTEM_STATE_DEGRADED:  return "DEGRADED";
        case SYSTEM_STATE_SAFE_OFF:  return "SAFE_OFF";
        case SYSTEM_STATE_EMERGENCY: return "EMERGENCY";
        default:                     return "UNKNOWN";
    }
}

static event_id_t state_to_event(system_state_t s)
{
    switch (s) {
        case SYSTEM_STATE_NORMAL:    return EVENT_ID_NORMAL;
        case SYSTEM_STATE_DEGRADED:  return EVENT_ID_DEGRADED;
        case SYSTEM_STATE_SAFE_OFF:  return EVENT_ID_SAFE_OFF;
        case SYSTEM_STATE_EMERGENCY: return EVENT_ID_EMERGENCY;
        default:                     return EVENT_ID_NONE;
    }
}

bool global_state_antiflap_allow(uint64_t now_ms)
{
    const antiflap_params_storage_t *cfg = config_get_antiflap();
    uint32_t cooldown_ms = cfg->cooldown_reentrada_s * MS_PER_SEC;
    uint32_t window_ms = cfg->janela_flap_s * MS_PER_SEC;
    uint32_t max_trans = cfg->max_transicoes_flap;

    if (s_last_transition_ms > 0 && (now_ms - s_last_transition_ms) < cooldown_ms) {
        return false;
    }
    if (s_flap_window_start_ms == 0 ||
        (now_ms - s_flap_window_start_ms) > window_ms) {
        s_flap_window_start_ms = now_ms;
        s_transition_count_in_window = 0;
    }
    s_transition_count_in_window++;
    return s_transition_count_in_window <= max_trans;
}

void global_state_antiflap_commit(uint64_t now_ms)
{
    s_last_transition_ms = now_ms;
}

void global_state_sync_from_config(global_state_t *gs)
{
    if (!gs) {
        return;
    }
    const system_params_storage_t *sys = config_get_system();
    gs->wizard_completed = sys->wizard_completed;
    gs->monitor_only_mode = sys->monitor_only_mode;
    gs->maintenance_mode = sys->maintenance_mode;
    gs->wizard_step = (wizard_step_t)config_get_wizard_step();
    snprintf(gs->config_schema_version, sizeof(gs->config_schema_version),
             "%s", CONFIG_ROOT_SCHEMA_VERSION);

    const calibration_params_storage_t *cal = config_get_calibration();
    for (int p = 1; p <= 10; p++) {
        acs712_set_zero_offset((uint8_t)p, cal->acs712_zero_offset_mv[p - 1]);
    }
    plug_manager_reload_limits();
}

esp_err_t global_state_init(void)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (!s_mutex) {
            return ESP_ERR_NO_MEM;
        }
    }
    s_last_transition_ms = 0;
    s_flap_window_start_ms = 0;
    s_transition_count_in_window = 0;
    return ESP_OK;
}

void global_state_bind(global_state_t *gs)
{
    s_gs = gs;
    (void)global_state_init();
}

esp_err_t global_state_get_snapshot(global_state_t *out)
{
    if (!out || !s_gs) {
        return ESP_ERR_INVALID_ARG;
    }
    mutex_lock();
    memcpy(out, s_gs, sizeof(*out));
    mutex_unlock();
    return ESP_OK;
}

const global_state_t *global_state_get_snapshot_ptr(void)
{
    return s_gs;
}

global_state_t *global_state_get_write_ptr(void)
{
    return s_gs;
}

esp_err_t global_state_set_health_flag(global_state_health_field_t field, bool ok)
{
    if (!s_gs) {
        return ESP_ERR_INVALID_STATE;
    }
    mutex_lock();
    switch (field) {
        case GS_HEALTH_TEMP:      s_gs->temp_ok = ok; break;
        case GS_HEALTH_ATO:      s_gs->ato_ok = ok; break;
        case GS_HEALTH_PZEM:      s_gs->pzem_ok = ok; break;
        case GS_HEALTH_SD:        s_gs->sd_ok = ok; break;
        case GS_HEALTH_WIFI:      s_gs->wifi_ok = ok; break;
        case GS_HEALTH_UI:        s_gs->ui_ok = ok; break;
        case GS_HEALTH_ELECTRIC:  s_gs->electric_ok = ok; break;
        case GS_HEALTH_SELFTEST:  s_gs->selftest_passed = ok; break;
        default:
            mutex_unlock();
            return ESP_ERR_INVALID_ARG;
    }
    mutex_unlock();
    return ESP_OK;
}

esp_err_t global_state_transition(system_state_t next_state, safeoff_reason_t reason,
                                   const char *source_alm, const char *source_module,
                                   uint64_t now_s)
{
    /* @requirement RF-GLOBAL-002 Autoridade única: delega às funções enter_* que
     * compartilham audit, relay OFF e safeoff_record com safety_controller.
     * Runtime loop usa safety_controller_evaluate → enter_* diretamente;
     * esta API é equivalente para callers explícitos (testes/futuro). */
    if (!s_gs) {
        return ESP_ERR_INVALID_STATE;
    }

    switch (next_state) {
        case SYSTEM_STATE_SAFE_OFF:
            return global_state_enter_safeoff(s_gs, reason, source_alm, source_module, now_s);
        case SYSTEM_STATE_EMERGENCY:
            return global_state_enter_emergency(s_gs, source_alm, source_module, now_s);
        case SYSTEM_STATE_DEGRADED:
            return global_state_enter_degraded(s_gs, source_module);
        case SYSTEM_STATE_NORMAL: {
            mutex_lock();
            if (s_gs->system_state == SYSTEM_STATE_EMERGENCY) {
                mutex_unlock();
                return ESP_ERR_INVALID_STATE;
            }
            if (s_gs->system_state == SYSTEM_STATE_SAFE_OFF) {
                mutex_unlock();
                return ESP_ERR_INVALID_STATE;
            }
            mutex_unlock();
            /* Anti-flap: responsabilidade do caller (safety_controller_evaluate). */
            return global_state_enter_normal(s_gs,
                source_module ? source_module : "global_state_transition");
        }
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

