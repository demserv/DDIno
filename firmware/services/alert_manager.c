#include "alert_manager.h"
#include "alm_ids.h"
#include <string.h>

static alert_slot_t s_slots[ALERT_SLOTS_MAX];

static int find_slot(int16_t alm_id)
{
    for (int i = 0; i < ALERT_SLOTS_MAX; i++) {
        if (s_slots[i].active && s_slots[i].alm_id == alm_id) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void)
{
    for (int i = 0; i < ALERT_SLOTS_MAX; i++) {
        if (!s_slots[i].active) {
            return i;
        }
    }
    return -1;
}

void alert_manager_init(void)
{
    for (int i = 0; i < ALERT_SLOTS_MAX; i++) {
        memset(&s_slots[i], 0, sizeof(alert_slot_t));
        s_slots[i].alm_id = -1;
    }
}

bool alert_manager_raise_full(int16_t alm_id, alert_severity_t sev, alert_category_t cat,
                               const char *msg, float val, const char *hint,
                               uint16_t related_plug, bool ack_req, bool state_associated, uint64_t ts)
{
    int idx = find_slot(alm_id);
    if (idx >= 0) {
        s_slots[idx].last_seen_ts = ts;
        s_slots[idx].value = val;
        return true;
    }

    idx = find_free_slot();
    if (idx < 0) return false;

    alert_slot_t *slot = &s_slots[idx];
    memset(slot, 0, sizeof(alert_slot_t));
    slot->alm_id = alm_id;
    slot->active = true;
    slot->ack_req = ack_req;
    slot->acked = false;
    slot->first_seen_ts = ts;
    slot->last_seen_ts = ts;
    slot->severity = sev;
    slot->category = cat;
    slot->value = val;
    slot->related_plug_id = related_plug;
    slot->state_associated = state_associated;
    if (msg) {
        strncpy(slot->message, msg, sizeof(slot->message) - 1);
        slot->message[sizeof(slot->message) - 1] = '\0';
    }
    if (hint) {
        strncpy(slot->action_hint, hint, sizeof(slot->action_hint) - 1);
        slot->action_hint[sizeof(slot->action_hint) - 1] = '\0';
    }
    return true;
}

bool alert_manager_raise(int16_t alm_id, bool ack_req, uint64_t ts)
{
    return alert_manager_raise_full(alm_id, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                                     NULL, 0.0f, NULL, 0, ack_req, false, ts);
}

bool alert_manager_ack(int16_t alm_id, uint64_t ts)
{
    int idx = find_slot(alm_id);
    if (idx < 0) return false;
    s_slots[idx].acked = true;
    s_slots[idx].ack_timestamp = ts;
    s_slots[idx].last_seen_ts = ts;
    return true;
}

bool alert_manager_is_active(int16_t alm_id)
{
    return (find_slot(alm_id) >= 0);
}

void alert_manager_clear(int16_t alm_id)
{
    int idx = find_slot(alm_id);
    if (idx >= 0) {
        memset(&s_slots[idx], 0, sizeof(alert_slot_t));
        s_slots[idx].alm_id = -1;
    }
}

uint16_t alert_manager_active_count(void)
{
    uint16_t c = 0;
    for (int i = 0; i < ALERT_SLOTS_MAX; i++) {
        if (s_slots[i].active) c++;
    }
    return c;
}

static bool is_critical_alm(int16_t alm_id)
{
    switch (alm_id) {
        case ALM_020: case ALM_026: case ALM_028: case ALM_029:
        case ALM_046: case ALM_048: case ALM_055: case ALM_060:
        case ALM_061: case ALM_062: case ALM_063: case ALM_065:
            return true;
        default:
            return false;
    }
}

uint16_t alert_manager_critical_count(void)
{
    uint16_t c = 0;
    for (int i = 0; i < ALERT_SLOTS_MAX; i++) {
        if (s_slots[i].active && is_critical_alm(s_slots[i].alm_id)) {
            c++;
        }
    }
    return c;
}

const alert_slot_t* alert_manager_get_slot(int16_t alm_id)
{
    int idx = find_slot(alm_id);
    if (idx < 0) return NULL;
    return &s_slots[idx];
}

void alert_manager_get_active_slots(alert_slot_t *out, uint16_t *count, uint16_t max)
{
    uint16_t written = 0;
    for (int i = 0; i < ALERT_SLOTS_MAX && written < max; i++) {
        if (s_slots[i].active) {
            out[written] = s_slots[i];
            written++;
        }
    }
    *count = written;
}
