#include "safety_gate.h"

#include "system_types.h"

safety_gate_result_t safety_gate_can_enable_automation(const global_state_t *gs)
{
    safety_gate_result_t deny = { .can_automate = false, .block_reason = NULL };
    safety_gate_result_t allow = { .can_automate = true,  .block_reason = NULL };

    if (!gs) {
        deny.block_reason = "INTERNAL_ERROR";
        return deny;
    }

    if (!gs->wizard_completed) {
        deny.block_reason = "WIZARD_NOT_COMPLETED";
        return deny;
    }

    if (gs->monitor_only_mode) {
        deny.block_reason = "MONITOR_ONLY_ACTIVE";
        return deny;
    }

    if (gs->system_state == SYSTEM_STATE_SAFE_OFF ||
        gs->system_state == SYSTEM_STATE_EMERGENCY) {
        deny.block_reason = "SAFE_MODE_ACTIVE";
        return deny;
    }

    if (!gs->selftest_passed) {
        deny.block_reason = "SELFTEST_NOT_PASSED";
        return deny;
    }

    if (!gs->hw_ok) {
        deny.block_reason = "HARDWARE_NOT_OK";
        return deny;
    }

    return allow;
}

