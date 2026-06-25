#ifndef FIRMWARE_FSM_RESTART_FSM_H
#define FIRMWARE_FSM_RESTART_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "plug_model.h"

typedef enum {
    RESTART_STATE_IDLE = 0,
    RESTART_STATE_WAITING,
    RESTART_STATE_ENERGIZING,
    RESTART_STATE_MONITORING,
    RESTART_STATE_COMPLETE
} restart_state_t;

typedef struct {
    uint32_t wait_time_ms;
    uint32_t stagger_interval_ms;
    uint32_t monitor_time_ms;
} restart_cfg_t;

typedef struct {
    restart_state_t state;
    restart_cfg_t cfg;
    uint64_t state_entered_ms;
    uint8_t next_relay_idx;
    bool complete;
} restart_fsm_t;

void restart_fsm_init(restart_fsm_t *fsm, const restart_cfg_t *cfg);
void restart_fsm_start(restart_fsm_t *fsm, uint64_t now_ms);
void restart_fsm_update(restart_fsm_t *fsm, uint64_t now_ms);
void restart_fsm_abort(restart_fsm_t *fsm);
bool restart_fsm_is_active(const restart_fsm_t *fsm);
bool restart_fsm_is_complete(const restart_fsm_t *fsm);
bool restart_fsm_should_energize(const restart_fsm_t *fsm, plug_id_t plug);
uint16_t restart_fsm_energized_mask(const restart_fsm_t *fsm);
restart_state_t restart_fsm_get_state(const restart_fsm_t *fsm);

#endif
