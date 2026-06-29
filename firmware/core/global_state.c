// @requirement RF-GLOBAL-001 Definição de estados globais
// @requirement RF-GLOBAL-002 Transições com prioridade e rastreabilidade
// @requirement RNF-GLOBAL-ANTIFLAP-001 Anti-flap em transições
// @requirement RF-SAFEOFF-CAUSE-001 Rastreamento de causa SAFE_OFF
#include "global_state.h"
#include "event_bus.h"
#include "audit_log.h"
#include "config_manager.h"
#include "driver_relay.h"
#include "hardware_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

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

static bool check_antiflap(uint64_t now_ms)
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
    if (!s_gs) {
        return ESP_ERR_INVALID_STATE;
    }

    uint64_t now_ms = now_s * MS_PER_SEC;

    mutex_lock();
    system_state_t prev = s_gs->system_state;

    if (prev == next_state) {
        mutex_unlock();
        return ESP_OK;
    }

    if (prev == SYSTEM_STATE_EMERGENCY && next_state == SYSTEM_STATE_NORMAL) {
        ESP_LOGW(TAG, "Transicao EMERGENCY->NORMAL bloqueada");
        mutex_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    if (next_state < prev && prev != SYSTEM_STATE_EMERGENCY) {
        if (next_state < SYSTEM_STATE_SAFE_OFF) {
            ESP_LOGW(TAG, "Transicao descendente invalida %s->%s",
                     state_to_str(prev), state_to_str(next_state));
            mutex_unlock();
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (!check_antiflap(now_ms)) {
        ESP_LOGW(TAG, "ANTIFLAP bloqueou %s->%s", state_to_str(prev), state_to_str(next_state));
        mutex_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    s_gs->system_state = next_state;

    if (next_state == SYSTEM_STATE_SAFE_OFF) {
        s_gs->safeoff_reason = reason;
        snprintf(s_gs->safeoff_entered_at, sizeof(s_gs->safeoff_entered_at),
                 "%llu", (unsigned long long)now_s);
        if (source_alm) {
            snprintf(s_gs->safeoff_source_alm, sizeof(s_gs->safeoff_source_alm),
                     "%s", source_alm);
        } else {
            s_gs->safeoff_source_alm[0] = '\0';
        }
        s_gs->electric_ok = false;
        relay_all_off();
    } else if (next_state == SYSTEM_STATE_EMERGENCY) {
        s_gs->electric_ok = false;
        relay_all_off();
    } else if (next_state == SYSTEM_STATE_NORMAL) {
        s_gs->safeoff_reason = SAFEOFF_REASON_NONE;
        s_gs->safeoff_source_alm[0] = '\0';
        s_gs->safeoff_entered_at[0] = '\0';
        s_gs->electric_ok = true;
    } else if (next_state == SYSTEM_STATE_DEGRADED) {
        if (prev == SYSTEM_STATE_SAFE_OFF || prev == SYSTEM_STATE_EMERGENCY) {
            s_gs->safeoff_reason = SAFEOFF_REASON_NONE;
            s_gs->safeoff_source_alm[0] = '\0';
        }
    }

    s_last_transition_ms = now_ms;
    mutex_unlock();

    audit_log_state_change(state_to_str(prev), state_to_str(next_state),
                           source_module ? source_module : "global_state_transition");
    event_bus_publish(state_to_event(next_state), NULL);

    ESP_LOGW(TAG, "GLOBAL_TRANSITION prev=%s next=%s reason=%d alm=%s module=%s ts=%llu",
             state_to_str(prev), state_to_str(next_state), (int)reason,
             source_alm ? source_alm : "-",
             source_module ? source_module : "-",
             (unsigned long long)now_s);

    return ESP_OK;
}
