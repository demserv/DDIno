// @requirement RNF-SECURITY-001 Validação de comandos: monitor_only, EMERGENCY, double_confirmation
// @requirement RF-PLUG-011 Dupla confirmação para plugs críticos P01/P02
#include "command_validator.h"
#include <stddef.h>
#include <string.h>

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

cmd_validation_t command_validator_can_set_config(const global_state_t *gs, const char *key)
{
    if (!gs) return denied("INTERNAL_ERROR");
    if (gs->system_state >= SYSTEM_STATE_SAFE_OFF) return denied("SAFE_MODE_ACTIVE");
    (void)key;
    cmd_validation_t ok = { .allowed = true, .requires_double_confirmation = false, .error_code = NULL };
    return ok;
}

cmd_validation_t command_validator_can_start_feed(const global_state_t *gs)
{
    if (!gs) return denied("INTERNAL_ERROR");
    if (gs->monitor_only_mode) return denied("MONITOR_ONLY_ACTIVE");
    if (gs->system_state >= SYSTEM_STATE_SAFE_OFF) return denied("SAFE_MODE_ACTIVE");
    if (gs->feed_active) return denied("FEED_ALREADY_ACTIVE");
    cmd_validation_t ok = { .allowed = true, .requires_double_confirmation = false, .error_code = NULL };
    return ok;
}

cmd_validation_t command_validator_can_restart(const global_state_t *gs)
{
    if (!gs) return denied("INTERNAL_ERROR");
    if (gs->system_state != SYSTEM_STATE_SAFE_OFF) return denied("NOT_IN_SAFE_OFF");
    if (gs->restart_in_progress) return denied("RESTART_ALREADY_IN_PROGRESS");
    cmd_validation_t ok = { .allowed = true, .requires_double_confirmation = true, .error_code = NULL };
    return ok;
}

cmd_validation_t command_validator_can_ack_alert(const global_state_t *gs, uint16_t alert_id)
{
    if (!gs) return denied("INTERNAL_ERROR");
    (void)alert_id;
    cmd_validation_t ok = { .allowed = true, .requires_double_confirmation = false, .error_code = NULL };
    return ok;
}

cmd_validation_t command_validator_can_set_mode(const global_state_t *gs, uint8_t plug_id)
{
    if (!gs) return denied("INTERNAL_ERROR");
    if (gs->system_state >= SYSTEM_STATE_SAFE_OFF) return denied("SAFE_MODE_ACTIVE");
    (void)plug_id;
    cmd_validation_t ok = { .allowed = true, .requires_double_confirmation = false, .error_code = NULL };
    return ok;
}

cmd_validation_t command_validator_can_calibrate(const global_state_t *gs)
{
    if (!gs) return denied("INTERNAL_ERROR");
    if (gs->system_state >= SYSTEM_STATE_SAFE_OFF) return denied("SAFE_MODE_ACTIVE");
    cmd_validation_t ok = { .allowed = true, .requires_double_confirmation = false, .error_code = NULL };
    return ok;
}
