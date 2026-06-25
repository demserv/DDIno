#include "command_validator.h"
#include <stddef.h>

static cmd_validation_t denied(const char *err)
{
    cmd_validation_t r = { .allowed = false, .requires_double_confirmation = false, .error_code = err };
    return r;
}

cmd_validation_t command_validator_can_toggle_plug(const global_state_t *gs, uint8_t plug_id, bool desired_on)
{
    (void)desired_on;

    if (!gs) return denied("INTERNAL_ERROR");

    if (gs->monitor_only_mode) {
        return denied("MONITOR_ONLY_ACTIVE");
    }

    if (gs->system_state == SYSTEM_STATE_EMERGENCY || gs->system_state == SYSTEM_STATE_SAFE_OFF) {
        return denied("SAFE_MODE_ACTIVE");
    }

    cmd_validation_t ok = { .allowed = true, .requires_double_confirmation = false, .error_code = NULL };

    if (plug_id == 1 || plug_id == 2) {
        ok.requires_double_confirmation = true;
    }

    return ok;
}
