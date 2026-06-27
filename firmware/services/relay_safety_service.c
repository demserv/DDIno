// @requirement RF-PLUG-011 Desligamento manual de plugs críticos com dupla confirmação
#include "relay_safety_service.h"
#include "driver_relay.h"
#include "global_state.h"
#include <stddef.h>

static bool is_p01_p02(uint8_t plug_id)
{
    return (plug_id == 1u || plug_id == 2u);
}

relay_apply_result_t relay_safety_compute(
    const relay_apply_request_t *req,
    bool *effective_on)
{
    if (req == NULL || effective_on == NULL) {
        return RELAY_APPLY_DENIED_STATE;
    }

    if (req->system_state == SYSTEM_STATE_EMERGENCY ||
        req->system_state == SYSTEM_STATE_SAFE_OFF) {
        *effective_on = false;
        return RELAY_APPLY_DENIED_STATE;
    }

    if (req->monitor_only_mode && req->desired_on) {
        *effective_on = false;
        return RELAY_APPLY_DENIED_MONITOR_ONLY;
    }

    if (is_p01_p02(req->plug_id) && !req->desired_on) {
        if (!req->critical_manual_confirmed) {
            *effective_on = true;
            return RELAY_APPLY_DENIED_CRITICAL_CONFIRMATION;
        }
    }

    *effective_on = req->desired_on;
    return RELAY_APPLY_OK;
}

esp_err_t relay_logical_on(uint8_t plug_id, bool critical_confirmed)
{
    const global_state_t *gs = global_state_get_snapshot();
    if (gs->system_state >= SYSTEM_STATE_SAFE_OFF) return ESP_ERR_INVALID_STATE;

    relay_apply_request_t req = {
        .system_state = gs->system_state,
        .monitor_only_mode = gs->monitor_only_mode,
        .plug_id = plug_id,
        .desired_on = true,
        .critical_manual_confirmed = critical_confirmed,
        .plug_is_critical_role = (plug_id == 1 || plug_id == 2)
    };
    bool effective = false;
    relay_apply_result_t r = relay_safety_compute(&req, &effective);
    if (r != RELAY_APPLY_OK) return ESP_ERR_INVALID_STATE;
    return relay_set(plug_id, true);
}

esp_err_t relay_logical_off(uint8_t plug_id, bool critical_confirmed)
{
    const global_state_t *gs = global_state_get_snapshot();
    if (gs->system_state >= SYSTEM_STATE_SAFE_OFF) return ESP_ERR_INVALID_STATE;

    relay_apply_request_t req = {
        .system_state = gs->system_state,
        .monitor_only_mode = gs->monitor_only_mode,
        .plug_id = plug_id,
        .desired_on = false,
        .critical_manual_confirmed = critical_confirmed,
        .plug_is_critical_role = (plug_id == 1 || plug_id == 2)
    };
    bool effective = false;
    relay_apply_result_t r = relay_safety_compute(&req, &effective);
    if (r != RELAY_APPLY_OK) return ESP_ERR_INVALID_STATE;
    return relay_set(plug_id, false);
}
