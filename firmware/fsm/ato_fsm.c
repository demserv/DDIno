#include "fsm/ato_fsm.h"
#include <string.h>

static void ato_reset_output(ato_output_t *o)
{
    memset(o, 0, sizeof(*o));
    o->state = ATO_STATE_NORMAL;
    o->safeoff_reason = SAFEOFF_REASON_NONE;
}

void ato_fsm_init(ato_fsm_t *fsm, const ato_params_t *cfg)
{
    if (!fsm || !cfg) return;
    memset(fsm, 0, sizeof(*fsm));
    fsm->cfg = *cfg;
    ato_reset_output(&fsm->out);
}

void ato_fsm_set_config(ato_fsm_t *fsm, const ato_params_t *cfg)
{
    if (!fsm || !cfg) return;
    fsm->cfg = *cfg;
}

void ato_fsm_clear_blocked(ato_fsm_t *fsm)
{
    if (!fsm) return;
    fsm->blocked_latched = false;
}

void ato_fsm_update(ato_fsm_t *fsm, const ato_input_t *in)
{
    if (!fsm || !in) return;
    ato_reset_output(&fsm->out);

    if (!fsm->cfg.enabled) {
        fsm->out.state = ATO_STATE_DISABLED;
        return;
    }

    if (!in->sample_valid) {
        fsm->out.state = ATO_STATE_ERROR;
        fsm->out.suggested_alm = ALM_017;
        return;
    }

    const int32_t low = fsm->cfg.low_level_adc;
    const int32_t high = fsm->cfg.high_level_adc;
    const int32_t overflow = high + fsm->cfg.overflow_margin_adc;
    const int32_t level = in->level_adc;

    if (level > overflow) {
        fsm->out.state = ATO_STATE_OVERFLOW;
        fsm->out.force_safe_off = true;
        fsm->out.safeoff_reason = SAFEOFF_REASON_ATO_OVERFLOW;
        fsm->out.suggested_alm = ALM_037;
        fsm->blocked_latched = true;
        return;
    }

    if (fsm->blocked_latched) {
        fsm->out.state = ATO_STATE_BLOCKED;
        fsm->out.suggested_alm = ALM_020;
        fsm->out.force_safe_off = true;
        fsm->out.safeoff_reason = SAFEOFF_REASON_ATO_OVERFLOW;
        return;
    }

    if (level < low) {
        fsm->out.state = ATO_STATE_REFILLING;
        fsm->out.pump_request_on = true;

        if (fsm->refill_started_ms == 0) {
            fsm->refill_started_ms = in->now_ms;
            fsm->out.suggested_alm = ALM_018;
            return;
        }

        const uint64_t elapsed_ms = in->now_ms - fsm->refill_started_ms;
        if (elapsed_ms > ((uint64_t)fsm->cfg.refill_timeout_s * 1000ULL)) {
            fsm->blocked_latched = true;
            fsm->out.state = ATO_STATE_BLOCKED;
            fsm->out.pump_request_on = false;
            fsm->out.force_safe_off = true;
            fsm->out.safeoff_reason = SAFEOFF_REASON_ATO_OVERFLOW;
            fsm->out.suggested_alm = ALM_019;
            return;
        }
        return;
    }

    fsm->refill_started_ms = 0;
    fsm->out.state = ATO_STATE_NORMAL;
}

const ato_output_t* ato_fsm_get_output(const ato_fsm_t *fsm)
{
    if (!fsm) return NULL;
    return &fsm->out;
}
