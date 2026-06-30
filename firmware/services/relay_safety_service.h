// @requirement RF-PLUG-011 Serviço de segurança de relés com dupla confirmação
#ifndef FIRMWARE_SERVICES_RELAY_SAFETY_SERVICE_H
#define FIRMWARE_SERVICES_RELAY_SAFETY_SERVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "system_types.h"

typedef enum {
    RELAY_APPLY_OK = 0,
    RELAY_APPLY_DENIED_STATE,
    RELAY_APPLY_DENIED_MONITOR_ONLY,
    RELAY_APPLY_DENIED_CRITICAL_CONFIRMATION
} relay_apply_result_t;

typedef struct {
    system_state_t system_state;
    bool monitor_only_mode;
    uint8_t plug_id;
    bool desired_on;
    bool critical_manual_confirmed;
    bool plug_is_critical_role;
} relay_apply_request_t;

/* @requirement RF-PLUG-011 Função pura de decisão (testável) para comutação de relé.
 * A atuação física é responsabilidade EXCLUSIVA de relay_abstraction_set (rota única);
 * este módulo não escreve no driver para não burlar o safety gate. */
relay_apply_result_t relay_safety_compute(
    const relay_apply_request_t *req,
    bool *effective_on);

#endif
