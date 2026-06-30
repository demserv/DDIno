// @requirement RF-FEED-001 a RF-FEED-003 Feed Mode FSM
// @requirement RF-FSM-FEED-001 Feed como camada ortogonal ao estado global
#ifndef FIRMWARE_FSM_FEED_FSM_H
#define FIRMWARE_FSM_FEED_FSM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    FEED_STATE_IDLE = 0,
    FEED_STATE_ACTIVE,
    FEED_STATE_COOLDOWN
} feed_state_t;

typedef struct {
    feed_state_t state;
    uint32_t duration_s;
    uint32_t cooldown_s;
    uint64_t state_started_ms;
    uint8_t feed_count_1h;
    uint64_t window_start_1h_ms;
    uint32_t pumps_off_mask;
    /* @requirement RF-PLUG-006 Estado real dos plugues no instante de ativação
     * (capturado pela camada de aplicação), persistido no snapshot. */
    uint32_t pre_feed_on_mask;
} feed_fsm_t;

void feed_fsm_init(feed_fsm_t *fsm, uint32_t duration_s, uint32_t cooldown_s);
bool feed_fsm_start(feed_fsm_t *fsm, uint64_t now_ms);
void feed_fsm_stop(feed_fsm_t *fsm, uint64_t now_ms);
/* @requirement RF-FEED-001 / RF-FSM-FEED-001 Aborto imediato (SAFE_OFF/EMERGENCY):
 * encerra o Feed sem cooldown, voltando a IDLE. */
void feed_fsm_abort(feed_fsm_t *fsm);
void feed_fsm_update(feed_fsm_t *fsm, uint64_t now_ms);
feed_state_t feed_fsm_get_state(const feed_fsm_t *fsm);
uint32_t feed_fsm_remaining_s(const feed_fsm_t *fsm, uint64_t now_ms);
bool feed_fsm_can_start(const feed_fsm_t *fsm, uint64_t now_ms);

#endif
