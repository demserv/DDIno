// @requirement RF-GLOBAL-SAFEOFF-EXIT-001 ACK manual obrigatório para sair de SAFE_OFF
// @requirement RF-GLOBAL-EMERG-EXIT-001 ACK manual para sair de EMERGENCY
#include "safe_state_ack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "services/alert_manager.h"

static bool s_safeoff_ack;
static bool s_emergency_ack;

static int16_t parse_source_alm(const char *alm_str)
{
    if (!alm_str || alm_str[0] == '\0') return -1;
    int id = 0;
    if (sscanf(alm_str, "ALM-%d", &id) == 1) return (int16_t)id;
    return -1;
}

static bool all_required_alerts_acked(uint64_t now_s)
{
    for (int16_t id = 1; id <= 65; id++) {
        const alert_slot_t *slot = alert_manager_get_slot(id);
        if (!slot || !slot->active) continue;
        if (!slot->ack_req) continue;
        if (slot->severity == ALERT_SEVERITY_CRITICAL || slot->severity == ALERT_SEVERITY_HIGH) {
            if (!slot->acked) return false;
            if (alert_manager_ext_double_ack_required(id) &&
                alert_manager_ext_get_ack_stage(id) != ACK_STAGE_CONFIRMED) {
                return false;
            }
        }
    }
    (void)now_s;
    return true;
}

void safe_state_ack_on_enter_safeoff(const global_state_t *gs)
{
    (void)gs;
    s_safeoff_ack = false;
}

void safe_state_ack_on_enter_emergency(void)
{
    s_emergency_ack = false;
}

void safe_state_ack_on_alert_ack(int16_t alm_id, uint64_t now_s)
{
    if (all_required_alerts_acked(now_s)) {
        s_safeoff_ack = true;
        s_emergency_ack = true;
        return;
    }
    const global_state_t *gs = NULL;
    extern global_state_t g_gs;
    gs = &g_gs;
    if (gs->system_state == SYSTEM_STATE_SAFE_OFF && gs->safeoff_source_alm[0] != '\0') {
        int16_t src = parse_source_alm(gs->safeoff_source_alm);
        if (src == alm_id) {
            const alert_slot_t *slot = alert_manager_get_slot(alm_id);
            if (slot && slot->acked) {
                if (!alert_manager_ext_double_ack_required(alm_id) ||
                    alert_manager_ext_get_ack_stage(alm_id) == ACK_STAGE_CONFIRMED) {
                    s_safeoff_ack = true;
                }
            }
        }
    }
}

bool safe_state_ack_manual_received(const global_state_t *gs)
{
    if (!gs) return false;
    if (gs->system_state == SYSTEM_STATE_EMERGENCY) {
        return s_emergency_ack;
    }
    if (gs->system_state == SYSTEM_STATE_SAFE_OFF) {
        return s_safeoff_ack;
    }
    return true;
}
