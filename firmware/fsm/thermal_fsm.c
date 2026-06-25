// @requirement RF-THERMAL-001 Leitura contínua com validação robusta
// @requirement RF-THERMAL-003 Classificação térmica por parâmetros
// @requirement RF-THERMAL-009 Exclusão mútua aquecedor e cooler
// @requirement RF-FSM-THERMAL-001 FSM térmica e impacto sistêmico
#include "fsm/thermal_fsm.h"
#include <string.h>

static void thermal_reset_output(thermal_output_t *o)
{
    memset(o, 0, sizeof(*o));
    o->state = THERMAL_STATE_NORMAL;
    o->safeoff_reason = SAFEOFF_REASON_NONE;
}

void thermal_fsm_init(thermal_fsm_t *fsm, const thermal_params_t *cfg)
{
    if (!fsm || !cfg) return;
    memset(fsm, 0, sizeof(*fsm));
    fsm->cfg = *cfg;
    thermal_reset_output(&fsm->out);
}

void thermal_fsm_set_config(thermal_fsm_t *fsm, const thermal_params_t *cfg)
{
    if (!fsm || !cfg) return;
    fsm->cfg = *cfg;
}

void thermal_fsm_update(thermal_fsm_t *fsm, const thermal_input_t *in)
{
    if (!fsm || !in) return;
    thermal_reset_output(&fsm->out);

    if (!in->sample_valid) {
        fsm->out.state = THERMAL_STATE_SENSOR_FAIL;
        fsm->out.sensor_fault = true;
        fsm->out.suggested_alm = ALM_013;
        return;
    }

    const float t = in->temp_c;
    const float sp = fsm->cfg.temp_normal_c;
    const float h  = fsm->cfg.hysteresis_c;

    if (fsm->cfg.extreme_enabled && t >= fsm->cfg.temp_extreme_c) {
        fsm->out.state = THERMAL_STATE_EXTREME;
        fsm->out.force_emergency = true;
        fsm->out.suggested_alm = ALM_028;
        return;
    }

    if (t >= fsm->cfg.temp_critical_c) {
        fsm->out.state = THERMAL_STATE_CRITICAL;
        fsm->out.force_safe_off = true;
        fsm->out.safeoff_reason = SAFEOFF_REASON_THERMAL_CRITICAL;
        fsm->out.suggested_alm = ALM_026;
        return;
    }

    bool want_heater = (t < (sp - h));
    bool want_cooler = (t > (sp + h));

    if (want_heater && want_cooler) {
        fsm->out.state = THERMAL_STATE_CRITICAL;
        fsm->out.force_safe_off = true;
        fsm->out.safeoff_reason = SAFEOFF_REASON_FSM_INVALID;
        fsm->out.suggested_alm = ALM_060;
        return;
    }

    if (want_cooler) {
        fsm->out.state = THERMAL_STATE_ALERT;
        fsm->out.request_cooler_on = true;
        fsm->out.suggested_alm = ALM_015;
    } else if (want_heater) {
        fsm->out.state = THERMAL_STATE_ALERT;
        fsm->out.request_heater_on = true;
    } else {
        fsm->out.state = THERMAL_STATE_NORMAL;
    }

}

const thermal_output_t* thermal_fsm_get_output(const thermal_fsm_t *fsm)
{
    if (!fsm) return NULL;
    return &fsm->out;
}
