// @requirement RF-GLOBAL-001 a RF-GLOBAL-004 Estados globais e transições
// @requirement RF-GLOBAL-SAFEOFF-EXIT-001 Saída segura de SAFE_OFF
// @requirement RF-GLOBAL-EMERG-EXIT-001 Saída controlada de EMERGENCY
#ifndef FIRMWARE_CORE_SAFETY_CONTROLLER_H
#define FIRMWARE_CORE_SAFETY_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "system_types.h"
#include "global_state.h"

typedef struct {
    bool emergency_condition;
    bool safeoff_condition;
    bool degraded_condition;
    safeoff_reason_t safeoff_reason_if_any;
    const char *safeoff_source_alm;
    const char *transition_cause;
    bool all_sensors_valid;
    bool selftest_passed;
    bool emergency_resolved;
    bool safeoff_cause_resolved;
    bool manual_ack_received;
    uint64_t cause_resolved_at_ms;
} safety_inputs_t;

typedef struct {
    uint64_t last_transition_ms;
    uint64_t flap_window_start_ms;
    uint32_t transition_count_in_window;
    uint64_t last_safeoff_exit_attempt_ms;
    uint32_t safeoff_stabilization_count;
} safety_context_t;

void safety_controller_init(global_state_t *gs);
void safety_controller_evaluate(global_state_t *gs, const safety_inputs_t *in, uint64_t now_s);
bool safety_controller_can_exit_safeoff(const global_state_t *gs, const safety_inputs_t *in, uint64_t now_s);
bool safety_controller_can_exit_emergency(const global_state_t *gs, const safety_inputs_t *in, uint64_t now_s);

esp_err_t global_state_enter_safeoff(global_state_t *gs, safeoff_reason_t reason,
                                     const char *source_alm, const char *source_module,
                                     uint64_t now_s);
esp_err_t global_state_enter_emergency(global_state_t *gs, const char *source_module, uint64_t now_s);
esp_err_t global_state_enter_degraded(global_state_t *gs, const char *source_module);

#endif
