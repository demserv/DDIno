#include "alert_manager_ext.h"
#include <string.h>

#define ACK_STATE_SLOTS_MAX 16

static alert_ack_state_t s_ack_states[ACK_STATE_SLOTS_MAX];

static int find_ack_state(int16_t alm_id)
{
    for (int i = 0; i < ACK_STATE_SLOTS_MAX; i++) {
        if (s_ack_states[i].alm_id == alm_id) return i;
    }
    return -1;
}

static int find_free_ack_state(void)
{
    for (int i = 0; i < ACK_STATE_SLOTS_MAX; i++) {
        if (s_ack_states[i].alm_id == 0) return i;
    }
    return -1;
}

bool alert_manager_ext_ack_critical(int16_t alm_id, uint64_t ts)
{
    const alert_slot_t *slot = alert_manager_get_slot(alm_id);
    if (!slot || !slot->active) return false;
    if (slot->severity != ALERT_SEVERITY_CRITICAL) {
        return alert_manager_ack(alm_id, ts);
    }

    int idx = find_ack_state(alm_id);
    if (idx < 0) {
        idx = find_free_ack_state();
        if (idx < 0) return false;
        s_ack_states[idx].alm_id = alm_id;
        s_ack_states[idx].ack_stage = ACK_STAGE_NONE;
        s_ack_states[idx].requires_double_ack = true;
    }

    if (s_ack_states[idx].ack_stage == ACK_STAGE_NONE) {
        s_ack_states[idx].ack_stage = ACK_STAGE_FIRST;
        s_ack_states[idx].first_ack_ts = ts;
        return true;
    }

    if (s_ack_states[idx].ack_stage == ACK_STAGE_FIRST) {
        s_ack_states[idx].ack_stage = ACK_STAGE_CONFIRMED;
        return alert_manager_ack(alm_id, ts);
    }

    return false;
}

uint8_t alert_manager_ext_get_ack_stage(int16_t alm_id)
{
    int idx = find_ack_state(alm_id);
    if (idx < 0) return ACK_STAGE_NONE;
    return s_ack_states[idx].ack_stage;
}

bool alert_manager_ext_is_critical_and_pending(int16_t alm_id)
{
    const alert_slot_t *slot = alert_manager_get_slot(alm_id);
    if (!slot || !slot->active) return false;
    if (slot->severity != ALERT_SEVERITY_CRITICAL) return false;
    uint8_t stage = alert_manager_ext_get_ack_stage(alm_id);
    return (stage != ACK_STAGE_CONFIRMED);
}

void alert_manager_ext_reset_ack_stage(int16_t alm_id)
{
    int idx = find_ack_state(alm_id);
    if (idx >= 0) {
        memset(&s_ack_states[idx], 0, sizeof(alert_ack_state_t));
    }
}

bool alert_manager_ext_double_ack_required(int16_t alm_id)
{
    const alert_slot_t *slot = alert_manager_get_slot(alm_id);
    if (!slot || !slot->active) return false;
    return (slot->severity == ALERT_SEVERITY_CRITICAL);
}
