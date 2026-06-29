// @requirement RF-PLUG-009 Religamento sequencial
// @requirement RF-PROTECTION-001 Religamento inteligente pós-SAFE_OFF
// @requirement RF-FSM-RELIG-ELECT-001 FSM de religamento elétrico inteligente
// @requirement RF-GLOBAL-REARM-001 Blocked plug isolation
#include "fsm/restart_fsm.h"

#include <string.h>

static const uint8_t RELAY_ENERGIZE_ORDER[PLUG_COUNT_TOTAL] = {
    4, 5, 1, 6, 2, 3, 7, 8, 9, 10
};

void restart_fsm_init(restart_fsm_t *fsm, const restart_cfg_t *cfg)
{
    if (!fsm) return;
    memset(fsm, 0, sizeof(*fsm));
    if (cfg) {
        fsm->cfg = *cfg;
    } else {
        fsm->cfg.wait_time_ms = 30000;
        fsm->cfg.stagger_interval_ms = 5000;
        fsm->cfg.monitor_time_ms = 10000;
    }
    fsm->state = RESTART_STATE_IDLE;
    fsm->complete = false;
    fsm->blocked_mask = 0;
}

void restart_fsm_start(restart_fsm_t *fsm, uint64_t now_ms)
{
    if (!fsm) return;
    fsm->state = RESTART_STATE_WAITING;
    fsm->state_entered_ms = now_ms;
    fsm->next_relay_idx = 0;
    fsm->complete = false;
    fsm->blocked_mask = 0;
}

void restart_fsm_update(restart_fsm_t *fsm, uint64_t now_ms)
{
    if (!fsm) return;
    if (fsm->state == RESTART_STATE_IDLE || fsm->state == RESTART_STATE_COMPLETE) return;

    uint64_t elapsed = now_ms - fsm->state_entered_ms;

    switch (fsm->state) {
        case RESTART_STATE_WAITING:
            if (elapsed >= fsm->cfg.wait_time_ms) {
                fsm->state = RESTART_STATE_ENERGIZING;
                fsm->state_entered_ms = now_ms;
                fsm->next_relay_idx = 0;
            }
            break;

        case RESTART_STATE_ENERGIZING:
            if (fsm->next_relay_idx >= PLUG_COUNT_TOTAL) {
                fsm->state = RESTART_STATE_MONITORING;
                fsm->state_entered_ms = now_ms;
            } else {
                uint64_t since_last = elapsed - fsm->next_relay_idx * fsm->cfg.stagger_interval_ms;
                if (since_last >= fsm->cfg.stagger_interval_ms) {
                    fsm->next_relay_idx++;
                    while (fsm->next_relay_idx < PLUG_COUNT_TOTAL) {
                        uint8_t plug_idx = RELAY_ENERGIZE_ORDER[fsm->next_relay_idx];
                        if (!(fsm->blocked_mask & (uint16_t)(1U << plug_idx))) break;
                        fsm->next_relay_idx++;
                    }
                }
            }
            break;

        case RESTART_STATE_MONITORING:
            if (elapsed >= fsm->cfg.monitor_time_ms) {
                fsm->state = RESTART_STATE_COMPLETE;
                fsm->complete = true;
            }
            break;

        default:
            break;
    }
}

void restart_fsm_abort(restart_fsm_t *fsm)
{
    if (!fsm) return;
    fsm->state = RESTART_STATE_IDLE;
    fsm->complete = false;
    fsm->next_relay_idx = 0;
}

bool restart_fsm_is_active(const restart_fsm_t *fsm)
{
    if (!fsm) return false;
    return fsm->state != RESTART_STATE_IDLE && fsm->state != RESTART_STATE_COMPLETE;
}

bool restart_fsm_is_complete(const restart_fsm_t *fsm)
{
    if (!fsm) return false;
    return fsm->complete;
}

bool restart_fsm_should_energize(const restart_fsm_t *fsm, plug_id_t plug)
{
    if (!fsm) return false;
    if (plug < 1 || plug > PLUG_COUNT_TOTAL) return false;
    if (fsm->state != RESTART_STATE_ENERGIZING) return false;
    uint8_t idx = (uint8_t)(plug - 1);
    for (uint8_t i = 0; i < fsm->next_relay_idx; i++) {
        if (RELAY_ENERGIZE_ORDER[i] == idx) return true;
    }
    return false;
}

void restart_fsm_set_blocked_mask(restart_fsm_t *fsm, uint16_t mask)
{
    if (!fsm) return;
    fsm->blocked_mask = mask;
}

uint16_t restart_fsm_energized_mask(const restart_fsm_t *fsm)
{
    if (!fsm) return 0;
    if (fsm->state != RESTART_STATE_ENERGIZING && fsm->state != RESTART_STATE_MONITORING && fsm->state != RESTART_STATE_COMPLETE) {
        return 0;
    }
    uint16_t mask = 0;
    uint8_t count = (fsm->state == RESTART_STATE_ENERGIZING) ? fsm->next_relay_idx : PLUG_COUNT_TOTAL;
    if (fsm->state == RESTART_STATE_MONITORING || fsm->state == RESTART_STATE_COMPLETE) {
        count = PLUG_COUNT_TOTAL;
    }
    for (uint8_t i = 0; i < count; i++) {
        uint8_t plug_idx = RELAY_ENERGIZE_ORDER[i];
        if (fsm->blocked_mask & (uint16_t)(1U << plug_idx)) continue;
        mask |= (uint16_t)(1U << plug_idx);
    }
    return mask;
}

restart_state_t restart_fsm_get_state(const restart_fsm_t *fsm)
{
    if (!fsm) return RESTART_STATE_IDLE;
    return fsm->state;
}

