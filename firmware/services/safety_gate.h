#ifndef SERVICES_SAFETY_GATE_H
#define SERVICES_SAFETY_GATE_H

#include <stdbool.h>
#include "global_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool can_automate;
    const char *block_reason;
} safety_gate_result_t;

safety_gate_result_t safety_gate_can_enable_automation(const global_state_t *gs);

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /* SERVICES_SAFETY_GATE_H */