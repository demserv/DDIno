// @requirement RF-GLOBAL-001 Definição de estados globais
// @requirement RF-GLOBAL-002 Transições de estado global com anti-flap
// @requirement RF-GLOBAL-003 Sinais sonoros/visuais em estado crítico
// @requirement RF-GLOBAL-004 Rastreamento de causa de SAFE_OFF
// @requirement RF-GLOBAL-SAFEOFF-EXIT-001 Pré-condições saída de SAFE_OFF
// @requirement RF-GLOBAL-EMERG-EXIT-001 Saída controlada de EMERGENCY
// @requirement RNF-GLOBAL-ANTIFLAP-001 Estabilização de retorno
#include "safety_controller.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "audit_log.h"
#include "config_manager.h"
#include "relay_abstraction.h"
#include "event_log.h"
#include "event_bus.h"
#include "global_state.h"
#include "hardware_config.h"
#include "services/safeoff_record.h"
#include "services/safe_state_ack.h"
#include "services/plug_manager.h"

static const char *TAG = "safety_controller";

static safety_context_t s_ctx;

void safety_controller_init(global_state_t *gs)
{
    if (!gs) return;
    memset(&s_ctx, 0, sizeof(s_ctx));
    gs->safeoff_reason = SAFEOFF_REASON_NONE;
    gs->safeoff_entered_at[0] = '\0';
    gs->safeoff_source_alm[0] = '\0';
    gs->electric_ok = true;
}

esp_err_t global_state_enter_safeoff(global_state_t *gs, safeoff_reason_t reason,
                                      const char *source_alm, const char *source_module,
                                      uint64_t now_s)
{
    if (!gs) return ESP_ERR_INVALID_ARG;
    if (reason == SAFEOFF_REASON_NONE) return ESP_ERR_INVALID_ARG;
    if (gs->system_state == SYSTEM_STATE_SAFE_OFF) return ESP_OK;

    system_state_t prev = gs->system_state;
    gs->system_state = SYSTEM_STATE_SAFE_OFF;
    gs->safeoff_reason = reason;
    snprintf(gs->safeoff_entered_at, sizeof(gs->safeoff_entered_at), "%llu",
             (unsigned long long)now_s);
    if (source_alm) {
        snprintf(gs->safeoff_source_alm, sizeof(gs->safeoff_source_alm), "%s", source_alm);
    } else {
        gs->safeoff_source_alm[0] = '\0';
    }
    gs->electric_ok = false;
    relay_abstraction_all_off();
    plug_manager_apply_safe_off();

    safeoff_record_append(reason, source_alm, now_s);
    safe_state_ack_on_enter_safeoff(gs);

    ESP_LOGE("safety", "SAFE_OFF entered from %d reason=%d alm=%s module=%s",
             (int)prev, (int)reason, source_alm ? source_alm : "?", source_module ? source_module : "?");
    audit_log_state_change(system_state_to_str(prev), "SAFE_OFF", source_module ? source_module : "enter_safeoff");
    return ESP_OK;
}

esp_err_t global_state_enter_emergency(global_state_t *gs, const char *source_alm,
                                      const char *source_module, uint64_t now_s)
{
    (void)now_s;
    if (!gs) return ESP_ERR_INVALID_ARG;
    if (gs->system_state >= SYSTEM_STATE_EMERGENCY) return ESP_OK;

    system_state_t prev = gs->system_state;
    gs->system_state = SYSTEM_STATE_EMERGENCY;
    gs->electric_ok = false;
    if (source_alm) {
        snprintf(gs->safeoff_source_alm, sizeof(gs->safeoff_source_alm), "%s", source_alm);
    } else {
        gs->safeoff_source_alm[0] = '\0';
    }
    relay_abstraction_all_off();
    plug_manager_apply_safe_off();
    safe_state_ack_on_enter_emergency();

    ESP_LOGE("safety", "EMERGENCY entered from %d alm=%s module=%s", (int)prev,
             source_alm ? source_alm : "?", source_module ? source_module : "?");
    audit_log_state_change(system_state_to_str(prev), "EMERGENCY", source_module ? source_module : "enter_emergency");
    return ESP_OK;
}

esp_err_t global_state_enter_degraded(global_state_t *gs, const char *source_module)
{
    if (!gs) return ESP_ERR_INVALID_ARG;
    if (gs->system_state >= SYSTEM_STATE_DEGRADED) return ESP_OK;

    system_state_t prev = gs->system_state;
    gs->system_state = SYSTEM_STATE_DEGRADED;
    ESP_LOGW("safety", "DEGRADED entered module=%s", source_module ? source_module : "?");
    audit_log_state_change(system_state_to_str(prev), "DEGRADED", source_module ? source_module : "enter_degraded");
    return ESP_OK;
}

esp_err_t global_state_enter_normal(global_state_t *gs, const char *source_module)
{
    if (!gs) return ESP_ERR_INVALID_ARG;
    if (gs->system_state == SYSTEM_STATE_NORMAL) return ESP_OK;

    system_state_t prev = gs->system_state;
    gs->system_state = SYSTEM_STATE_NORMAL;
    gs->safeoff_reason = SAFEOFF_REASON_NONE;
    gs->safeoff_source_alm[0] = '\0';
    gs->electric_ok = true;
    ESP_LOGI("safety", "NORMAL entered from %s module=%s",
             system_state_to_str(prev), source_module ? source_module : "?");
    audit_log_state_change(system_state_to_str(prev), "NORMAL", source_module ? source_module : "enter_normal");
    event_bus_publish(EVENT_ID_NORMAL, NULL);
    return ESP_OK;
}

void safety_controller_evaluate(global_state_t *gs, const safety_inputs_t *in, uint64_t now_s)
{
    if (!gs || !in) return;

    /* @requirement RF-GLOBAL-002 Prioridade EMERGENCY > SAFE_OFF > DEGRADED > NORMAL.
     * Esta função muta o gs recebido (contrato testável) e replica os efeitos
     * centrais: relay_abstraction_all_off em SAFE_OFF/EMERGENCY e audit trail.
     * Saída de SAFE_OFF/EMERGENCY nunca é automática (ver can_exit_*). */
    const system_state_t prev = gs->system_state;
    system_state_t next;

    if (in->emergency_condition) {
        next = SYSTEM_STATE_EMERGENCY;
    } else if (in->safeoff_condition) {
        next = SYSTEM_STATE_SAFE_OFF;
    } else if (in->degraded_condition || !in->all_sensors_valid || !in->selftest_passed) {
        /* Não rebaixar SAFE_OFF/EMERGENCY para DEGRADED automaticamente. */
        if (prev == SYSTEM_STATE_SAFE_OFF || prev == SYSTEM_STATE_EMERGENCY) return;
        next = SYSTEM_STATE_DEGRADED;
    } else {
        /* @requirement RF-GLOBAL-SAFEOFF-EXIT-001 / RF-GLOBAL-EMERG-EXIT-001
         * Retorno a NORMAL somente a partir de NORMAL/DEGRADED. */
        if (prev != SYSTEM_STATE_NORMAL && prev != SYSTEM_STATE_DEGRADED) return;
        next = SYSTEM_STATE_NORMAL;
    }

    if (next == prev) return;

    /* EMERGENCY não rebaixa automaticamente. */
    if (prev == SYSTEM_STATE_EMERGENCY && next != SYSTEM_STATE_EMERGENCY) return;

    bool is_recovery = (next == SYSTEM_STATE_NORMAL || next == SYSTEM_STATE_DEGRADED);
    uint64_t now_ms = now_s * MS_PER_SEC;
    if (is_recovery && !global_state_antiflap_allow(now_ms)) {
        const antiflap_params_storage_t *cfg = config_get_antiflap();
        ESP_LOGW(TAG, "ANTIFLAP: transicao %s->%s bloqueada (max %lu em %lus)",
                 system_state_to_str(prev), system_state_to_str(next),
                 (unsigned long)cfg->max_transicoes_flap, (unsigned long)cfg->janela_flap_s);
        return;
    }

    const char *cause = in->transition_cause ? in->transition_cause : "safety_controller";

    safeoff_reason_t so_reason = in->safeoff_reason_if_any;
    if (next == SYSTEM_STATE_SAFE_OFF && so_reason == SAFEOFF_REASON_NONE) {
        ESP_LOGW(TAG, "SAFE_OFF bloqueado: safeoff_reason_if_any ausente");
        return;
    }

    const char *alm_tag = in->safeoff_source_alm;
    if (next == SYSTEM_STATE_EMERGENCY && in->emergency_source_alm) {
        alm_tag = in->emergency_source_alm;
    }
    esp_err_t tr = global_state_transition(next, so_reason, alm_tag, cause, now_s);
    if (tr != ESP_OK) {
        return;
    }

    global_state_antiflap_commit(now_ms);

    ESP_LOGW(TAG, "GLOBAL_TRANSITION prev=%s next=%s cause=%s",
             system_state_to_str(prev), system_state_to_str(next), cause);
}

bool safety_controller_can_exit_safeoff(const global_state_t *gs, const safety_inputs_t *in, uint64_t now_s)
{
    if (gs->system_state != SYSTEM_STATE_SAFE_OFF) return false;
    if (!in) return false;
    if (in->emergency_condition) return false;
    if (!in->safeoff_cause_resolved) return false;
    if (!in->all_sensors_valid) return false;
    if (!in->selftest_passed) return false;
    if (!in->manual_ack_received) return false;

    if (in->cause_resolved_at_ms > 0) {
        uint64_t elapsed = (now_s * MS_PER_SEC) - in->cause_resolved_at_ms;
        if (elapsed < (HW_SAFEOFF_CAUSE_STABLE_S * MS_PER_SEC)) return false;
    }

    return true;
}

bool safety_controller_can_exit_emergency(const global_state_t *gs, const safety_inputs_t *in, uint64_t now_s)
{
    if (gs->system_state != SYSTEM_STATE_EMERGENCY) return false;
    if (!in) return false;
    if (!in->emergency_resolved) return false;
    if (!in->all_sensors_valid) return false;
    if (!in->manual_ack_received) return false;

    if (in->cause_resolved_at_ms > 0) {
        uint64_t elapsed = (now_s * MS_PER_SEC) - in->cause_resolved_at_ms;
        if (elapsed < (HW_EMERGENCY_CAUSE_STABLE_S * MS_PER_SEC)) return false;
    }

    return true;
}

