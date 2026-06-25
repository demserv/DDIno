#ifndef FIRMWARE_FSM_ATO_FSM_H
#define FIRMWARE_FSM_ATO_FSM_H

#include <stdbool.h>
#include <stdint.h>
#include "system_types.h"
#include "alm_ids.h"

typedef enum {
    ATO_STATE_NORMAL = 0,
    ATO_STATE_REFILLING,
    ATO_STATE_ERROR,
    ATO_STATE_BLOCKED,
    ATO_STATE_OVERFLOW,
    ATO_STATE_DISABLED
} ato_state_t;

typedef struct {
    bool     enabled;
    int32_t  low_level_adc;
    int32_t  high_level_adc;
    int32_t  overflow_margin_adc;
    uint32_t refill_timeout_s;
} ato_params_t;

typedef struct {
    bool     sample_valid;
    int32_t  level_adc;
    uint64_t now_ms;
} ato_input_t;

typedef struct {
    ato_state_t state;
    bool pump_request_on;
    bool force_safe_off;
    safeoff_reason_t safeoff_reason;
    uint16_t suggested_alm;
} ato_output_t;

typedef struct {
    ato_params_t cfg;
    ato_output_t out;
    bool blocked_latched;
    uint64_t refill_started_ms;
} ato_fsm_t;

void ato_fsm_init(ato_fsm_t *fsm, const ato_params_t *cfg);
void ato_fsm_set_config(ato_fsm_t *fsm, const ato_params_t *cfg);
void ato_fsm_update(ato_fsm_t *fsm, const ato_input_t *in);
void ato_fsm_clear_blocked(ato_fsm_t *fsm);
const ato_output_t* ato_fsm_get_output(const ato_fsm_t *fsm);

#endif
