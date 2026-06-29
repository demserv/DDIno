// @requirement RF-ATO-001 Leitura de nível com debounce ADC temporal (3 amostras)
// @requirement RF-ATO-DIGITAL-001 ATO digital ON/OFF com histerese ADC
// @requirement RF-ATO-002 FSM de proteção ATO com seis estados
// @requirement RF-ATO-004 Detecção de padrão anormal de refill
// @requirement RF-ATO-005 Detecção de reservatório vazio ou bloqueio
// @requirement RF-FSM-ATO-001 Impacto dos estados ATO no estado global
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
    fsm->debounce_required = 3;
    fsm->debounce_count = 0;
    fsm->last_stable_level = -1;
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
        fsm->debounce_count = 0;
        return;
    }

    const int32_t low = fsm->cfg.low_level_adc;
    const int32_t high = fsm->cfg.high_level_adc;
    const int32_t overflow = high + fsm->cfg.overflow_margin_adc;
    const int32_t level = in->level_adc;

    if (fsm->last_stable_level < 0) {
        fsm->last_stable_level = level;
    }

    if (level > overflow) {
        fsm->debounce_count = 0;
        fsm->last_stable_level = level;
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
        fsm->debounce_count = 0;
        return;
    }

    int32_t diff = level - fsm->last_stable_level;
    if (diff < 0) diff = -diff;
    if (diff < 50) {
        fsm->debounce_count++;
    } else {
        fsm->debounce_count = 0;
        fsm->last_stable_level = level;
    }
    if (fsm->debounce_count < fsm->debounce_required) {
        fsm->out.state = ATO_STATE_NORMAL;
        return;
    }
    fsm->last_stable_level = level;

    if (level < low) {
        fsm->out.state = ATO_STATE_REFILLING;
        fsm->out.pump_request_on = true;

        if (fsm->refill_started_ms == 0) {
            fsm->refill_started_ms = in->now_ms;
            fsm->out.suggested_alm = ALM_018;
            return;
        }

        const uint64_t elapsed_ms = in->now_ms - fsm->refill_started_ms;
        if (elapsed_ms > ((uint64_t)fsm->cfg.refill_timeout_s * MS_PER_SEC)) {
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
    fsm->debounce_count = 0;
}

const ato_output_t* ato_fsm_get_output(const ato_fsm_t *fsm)
{
    if (!fsm) return NULL;
    return &fsm->out;
}
