#include "safety_controller.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "safety_controller";

static safety_context_t s_ctx;

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

void safety_controller_init(global_state_t *gs)
{
    if (!gs) return;
    memset(&s_ctx, 0, sizeof(s_ctx));
    gs->safeoff_reason = SAFEOFF_REASON_NONE;
    gs->safeoff_entered_at[0] = '\0';
    gs->safeoff_source_alm[0] = '\0';
}

static bool check_antiflap(system_state_t next, uint64_t now_ms)
{
    if (s_ctx.last_transition_ms == 0) return true;
    uint64_t elapsed = now_ms - s_ctx.last_transition_ms;
    if (elapsed < 3000) return false;

    if (s_ctx.flap_window_start_ms == 0) {
        s_ctx.flap_window_start_ms = now_ms;
        s_ctx.transition_count_in_window = 0;
    }
    if ((now_ms - s_ctx.flap_window_start_ms) > 60000) {
        s_ctx.flap_window_start_ms = now_ms;
        s_ctx.transition_count_in_window = 0;
    }
    s_ctx.transition_count_in_window++;
    if (s_ctx.transition_count_in_window > 3) return false;
    return true;
}

void safety_controller_evaluate(global_state_t *gs, const safety_inputs_t *in, uint64_t now_s)
{
    if (!gs || !in) return;

    system_state_t prev = gs->system_state;
    system_state_t next = SYSTEM_STATE_NORMAL;

    if (in->emergency_condition) {
        next = SYSTEM_STATE_EMERGENCY;
    } else if (in->safeoff_condition) {
        next = SYSTEM_STATE_SAFE_OFF;
    } else if (in->degraded_condition || !in->all_sensors_valid) {
        next = SYSTEM_STATE_DEGRADED;
    } else {
        next = SYSTEM_STATE_NORMAL;
    }

    if (next == prev) return;

    if (!check_antiflap(next, now_s * 1000ULL)) {
        ESP_LOGW(TAG, "ANTIFLAP: transicao %s->%s bloqueada (muitas transicoes)",
                 state_to_str(prev), state_to_str(next));
        return;
    }

    if (next == SYSTEM_STATE_SAFE_OFF) {
        gs->safeoff_reason = in->safeoff_reason_if_any;
        snprintf(gs->safeoff_entered_at, sizeof(gs->safeoff_entered_at), "%llu",
                 (unsigned long long)now_s);
        if (in->safeoff_source_alm) {
            snprintf(gs->safeoff_source_alm, sizeof(gs->safeoff_source_alm), "%s", in->safeoff_source_alm);
        } else {
            gs->safeoff_source_alm[0] = '\0';
        }
    }

    if (next == SYSTEM_STATE_NORMAL || next == SYSTEM_STATE_DEGRADED) {
        gs->safeoff_reason = SAFEOFF_REASON_NONE;
        gs->safeoff_source_alm[0] = '\0';
    }

    gs->system_state = next;
    s_ctx.last_transition_ms = now_s * 1000ULL;

    ESP_LOGW(TAG, "GLOBAL_TRANSITION prev=%s next=%s cause=%s",
             state_to_str(prev), state_to_str(next),
             in->transition_cause ? in->transition_cause : "N/A");
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
        uint64_t elapsed = (now_s * 1000ULL) - in->cause_resolved_at_ms;
        if (elapsed < 10000) return false;
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
        uint64_t elapsed = (now_s * 1000ULL) - in->cause_resolved_at_ms;
        if (elapsed < 30000) return false;
    }

    return true;
}
