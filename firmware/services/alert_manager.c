// @requirement RF-ALERT-002 Silence enforcement
// @requirement RF-ALERT-004 ACK timeout enforcement
// @requirement RF-ALERT-001 Modelo canônico de alerta
#include "alert_manager.h"

#include <string.h>

#include "alm_ids.h"
#include "alm_catalog.h"

static alert_slot_t s_slots[ALERT_SLOTS_MAX];

#define ALERT_HISTORY_MAX 16
static alert_slot_t s_history[ALERT_HISTORY_MAX];
static uint16_t s_history_count;

static void history_push(const alert_slot_t *slot)
{
    if (!slot) return;
    uint16_t idx = s_history_count < ALERT_HISTORY_MAX ? s_history_count : (ALERT_HISTORY_MAX - 1);
    if (s_history_count >= ALERT_HISTORY_MAX) {
        memmove(&s_history[0], &s_history[1], (ALERT_HISTORY_MAX - 1) * sizeof(alert_slot_t));
        idx = ALERT_HISTORY_MAX - 1;
    } else {
        s_history_count++;
    }
    s_history[idx] = *slot;
    s_history[idx].active = false;
}

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
    memset(s_history, 0, sizeof(s_history));
    s_history_count = 0;
}

static bool slot_is_silenced(const alert_slot_t *slot, uint64_t now_ts)
{
    return (slot->silenced_until > 0 && now_ts < slot->silenced_until);
}

bool alert_manager_raise_full(int16_t alm_id, alert_severity_t sev, alert_category_t cat,
                               const char *msg, float val, const char *hint,
                               uint16_t related_plug, bool ack_req, bool state_associated, uint64_t ts)
{
    int idx = find_slot(alm_id);
    if (idx >= 0) {
        /* @requirement RF-ALERT-002/005 Silence suprime apenas som/repetição sonora
         * (camada do buzzer); o alerta continua ativo e atualizado, nunca é descartado. */
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
    /* @requirement RF-ALERT-001/003 A forma curta consulta a tabela canônica de ALMs
     * para severidade, categoria, ack_req e action_hint, em vez de assumir WARNING/NULL.
     * O parâmetro ack_req do chamador é honrado apenas quando a tabela não exige ACK. */
    const alm_meta_t *meta = alm_catalog_get(alm_id);
    if (meta) {
        bool effective_ack = ack_req || meta->ack_req;
        return alert_manager_raise_full(alm_id, meta->severity, meta->category,
                                        NULL, 0.0f, meta->action_hint, 0,
                                        effective_ack, false, ts);
    }
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
        history_push(&s_slots[idx]);
        alert_manager_ext_reset_ack_stage(alm_id);
        memset(&s_slots[idx], 0, sizeof(alert_slot_t));
        s_slots[idx].alm_id = -1;
    }
}

void alert_manager_try_auto_clear(int16_t alm_id, bool condition_ok)
{
    if (!condition_ok) return;
    const alert_slot_t *slot = alert_manager_get_slot(alm_id);
    if (!slot || !slot->active) return;
    if (slot->ack_req && !slot->acked) return;
    const alm_meta_t *meta = alm_catalog_get(alm_id);
    if (meta && meta->auto_clear) {
        alert_manager_clear(alm_id);
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

void alert_manager_check_ack_timeout(uint64_t now_s, uint32_t timeout_s)
{
    for (int i = 0; i < ALERT_SLOTS_MAX; i++) {
        alert_slot_t *slot = &s_slots[i];
        if (slot->active && slot->ack_req && !slot->acked) {
            if ((now_s - slot->first_seen_ts) > timeout_s) {
                if (slot->severity == ALERT_SEVERITY_CRITICAL || slot->severity == ALERT_SEVERITY_HIGH) {
                    const alm_meta_t *meta = alm_catalog_get(ALM_046);
                    const char *hint = (meta && meta->action_hint[0]) ? meta->action_hint
                                      : "Reconheca o alerta critico pendente";
                    alert_manager_raise_full(ALM_046, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_SYSTEM,
                                             "ACK timeout escalation", 0.0f, hint,
                                             0, true, false, now_s);
                }
            }
        }
    }
}

static bool is_critical_alm(int16_t alm_id)
{
    switch (alm_id) {
        /* @requirement Errata 2026-07-01: ALM-062 (acesso não autorizado Web) é
         * apenas alarme SECURITY/WARNING — NÃO é crítico e NÃO escala para SAFE_OFF. */
        case ALM_020: case ALM_026: case ALM_028: case ALM_029:
        case ALM_046: case ALM_048: case ALM_055: case ALM_060:
        case ALM_061: case ALM_063: case ALM_065:
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
    /* @requirement RF-ALERT-002/005 Todos os alertas ativos permanecem visíveis,
     * inclusive silenciados (a UI pode marcá-los via silenced_until). Crítico nunca
     * é ocultado. Silence ≠ resolução. */
    uint16_t written = 0;
    for (int i = 0; i < ALERT_SLOTS_MAX && written < max; i++) {
        if (s_slots[i].active) {
            out[written] = s_slots[i];
            written++;
        }
    }
    *count = written;
}

void alert_manager_get_history_slots(alert_slot_t *out, uint16_t *count, uint16_t max)
{
    uint16_t written = 0;
    for (int i = (int)s_history_count - 1; i >= 0 && written < max; i--) {
        out[written++] = s_history[i];
    }
    *count = written;
}

void alert_manager_set_silenced(int16_t alm_id, uint64_t until_ts)
{
    int idx = find_slot(alm_id);
    if (idx >= 0) {
        s_slots[idx].silenced_until = until_ts;
    }
}

bool alert_manager_is_silenced(int16_t alm_id, uint64_t now_ts)
{
    int idx = find_slot(alm_id);
    if (idx < 0) return false;
    return slot_is_silenced(&s_slots[idx], now_ts);
}

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

bool alert_manager_ack_with_policy(int16_t alm_id, uint64_t ts)
{
    if (!alert_manager_is_active(alm_id)) return false;
    if (alert_manager_ext_double_ack_required(alm_id)) {
        return alert_manager_ext_ack_critical(alm_id, ts);
    }
    return alert_manager_ack(alm_id, ts);
}

