#include "relay_safety_service.h"
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
