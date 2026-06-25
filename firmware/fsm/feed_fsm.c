// @requirement RF-FEED-001 Comportamento dos plugues em Feed Mode
// @requirement RF-FEED-002 Interface obrigatória do Feed Mode
// @requirement RF-FEED-003 Sinalização visual do Feed Mode
// @requirement RF-FSM-FEED-001 Feed Mode como camada ortogonal
#include "fsm/feed_fsm.h"

#define MAX_FEEDS_PER_HOUR 2

void feed_fsm_init(feed_fsm_t *fsm, uint32_t duration_s, uint32_t cooldown_s)
{
    if (!fsm) return;
    fsm->state = FEED_STATE_IDLE;
    fsm->duration_s = duration_s;
    fsm->cooldown_s = cooldown_s;
    fsm->state_started_ms = 0;
    fsm->feed_count_1h = 0;
    fsm->window_start_1h_ms = 0;
    fsm->pumps_off_mask = 0x0F;
}

bool feed_fsm_can_start(const feed_fsm_t *fsm, uint64_t now_ms)
{
    if (!fsm) return false;
    if (fsm->state != FEED_STATE_IDLE) return false;
    if (fsm->feed_count_1h >= MAX_FEEDS_PER_HOUR) {
        uint64_t elapsed = now_ms - fsm->window_start_1h_ms;
        if (elapsed < 3600000ULL) return false;
    }
    return true;
}

bool feed_fsm_start(feed_fsm_t *fsm, uint64_t now_ms)
{
    if (!fsm) return false;
    if (!feed_fsm_can_start(fsm, now_ms)) return false;

    fsm->state = FEED_STATE_ACTIVE;
    fsm->state_started_ms = now_ms;

    if (fsm->feed_count_1h == 0) {
        fsm->window_start_1h_ms = now_ms;
    }
    fsm->feed_count_1h++;

    return true;
}

void feed_fsm_stop(feed_fsm_t *fsm)
{
    if (!fsm) return;
    fsm->state = FEED_STATE_COOLDOWN;
    fsm->state_started_ms = 0;
}

void feed_fsm_update(feed_fsm_t *fsm, uint64_t now_ms)
{
    if (!fsm) return;

    switch (fsm->state) {
        case FEED_STATE_ACTIVE:
            if (fsm->state_started_ms > 0) {
                uint64_t elapsed = now_ms - fsm->state_started_ms;
                if (elapsed >= (uint64_t)fsm->duration_s * 1000ULL) {
                    feed_fsm_stop(fsm);
                }
            }
            break;

        case FEED_STATE_COOLDOWN:
            if (fsm->cooldown_s > 0) {
                uint64_t elapsed = now_ms - fsm->state_started_ms;
                if (elapsed >= (uint64_t)fsm->cooldown_s * 1000ULL) {
                    fsm->state = FEED_STATE_IDLE;
                    fsm->state_started_ms = 0;
                }
            } else {
                fsm->state = FEED_STATE_IDLE;
                fsm->state_started_ms = 0;
            }
            break;

        default:
            break;
    }

    if (fsm->feed_count_1h > 0) {
        uint64_t elapsed = now_ms - fsm->window_start_1h_ms;
        if (elapsed >= 3600000ULL) {
            fsm->feed_count_1h = 0;
            fsm->window_start_1h_ms = 0;
        }
    }
}

feed_state_t feed_fsm_get_state(const feed_fsm_t *fsm)
{
    return fsm ? fsm->state : FEED_STATE_IDLE;
}

uint32_t feed_fsm_remaining_s(const feed_fsm_t *fsm)
{
    if (!fsm || fsm->state != FEED_STATE_ACTIVE) return 0;
    return fsm->duration_s;
}
