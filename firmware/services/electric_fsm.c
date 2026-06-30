// @requirement RF-ENERGY-007 Proteção de sobretensão e subtensão
// @requirement RF-ENERGY-008 Proteção de sobrecorrente total
// @requirement RF-ENERGY-009 Monitoramento de fator de potência
// @requirement RF-PLUG-014 Curto-circuito e sobrecarga extrema por plugue
// @requirement RF-FSM-ELECTRIC-001 FSM de proteção elétrica
// @requirement RNF-ELECTRICAL-002 Hierarquia de proteções elétricas
#include "services/electric_fsm.h"
#include "hardware_config.h"
#include <string.h>

static void electric_reset_output(electric_output_t *o)
{
    memset(o, 0, sizeof(*o));
    o->state = ELECTRIC_STATE_NORMAL;
    o->safeoff_reason = SAFEOFF_REASON_NONE;
    o->shorted_plug_id = 0;
}

void electric_fsm_init(electric_fsm_t *fsm, const electric_params_t *cfg)
{
    if (!fsm || !cfg) return;
    memset(fsm, 0, sizeof(*fsm));
    fsm->cfg = *cfg;
    electric_reset_output(&fsm->out);
}

void electric_fsm_update(electric_fsm_t *fsm, const electric_input_t *in)
{
    if (!fsm || !in) return;
    electric_reset_output(&fsm->out);

    if (!in->sample_valid) {
        fsm->out.state = ELECTRIC_STATE_SENSOR_FAIL;
        fsm->out.suggested_alm = ALM_029;
        return;
    }

    fsm->out.measured_total_power_w = in->total_power_w;

    /* @requirement RF-PLUG-014 Detecção de curto-circuito por janela temporal
     * (tempo_deteccao_curto_ms): a corrente do plugue deve permanecer acima de
     * fator_curto × limite por todo o intervalo configurado antes de declarar curto.
     * O plugue afetado é sinalizado em shorted_plug_id para bloqueio/isolamento. */
    for (uint8_t i = 0; i < in->plug_count && i < 10; i++) {
        float limit = fsm->cfg.fator_curto * fsm->cfg.per_plug_current_limit_a;
        if (in->plug_currents_a[i] > limit) {
            if (fsm->plug_short_start_ms[i] == 0) {
                fsm->plug_short_start_ms[i] = in->now_ms;
            }
            uint64_t elapsed_ms = in->now_ms - fsm->plug_short_start_ms[i];
            if (elapsed_ms >= fsm->cfg.tempo_deteccao_curto_ms) {
                fsm->out.state = ELECTRIC_STATE_SHORT_CIRCUIT;
                fsm->out.force_safe_off = true;
                fsm->out.safeoff_reason = SAFEOFF_REASON_PLUG_SHORT;
                fsm->out.suggested_alm = ALM_055;
                fsm->out.shorted_plug_id = i + 1;
                return;
            }
        } else {
            fsm->plug_short_start_ms[i] = 0;
        }
    }

    /* Standard overcurrent per plug */
    for (uint8_t i = 0; i < in->plug_count && i < 10; i++) {
        if (in->plug_currents_a[i] > fsm->cfg.per_plug_current_limit_a) {
            fsm->out.state = ELECTRIC_STATE_OVERLOAD;
            fsm->out.force_safe_off = true;
            fsm->out.safeoff_reason = SAFEOFF_REASON_PLUG_SHORT;
            fsm->out.suggested_alm = ALM_055;
            fsm->out.shorted_plug_id = i + 1;
            return;
        }
    }

    /* Overvoltage detection with persistence */
    if (in->voltage_v > fsm->cfg.overvoltage_limit_v) {
        if (fsm->overvoltage_start_ms == 0) {
            fsm->overvoltage_start_ms = in->now_ms;
        }
        uint64_t elapsed = (in->now_ms - fsm->overvoltage_start_ms) / MS_PER_SEC;
        if (elapsed >= fsm->cfg.overvoltage_time_s) {
            fsm->out.state = ELECTRIC_STATE_OVERVOLTAGE;
            fsm->out.force_safe_off = true;
            fsm->out.safeoff_reason = SAFEOFF_REASON_OVERVOLTAGE;
            /* @requirement RF-ENERGY-007 ALM-050 = Sobretensão de rede (tabela ALM SRS). */
            fsm->out.suggested_alm = ALM_050;
            return;
        }
        fsm->out.state = ELECTRIC_STATE_OVERVOLTAGE;
    } else {
        fsm->overvoltage_start_ms = 0;
    }

    /* Undervoltage detection with persistence */
    if (in->voltage_v > 0.1f && in->voltage_v < fsm->cfg.undervoltage_limit_v) {
        if (fsm->undervoltage_start_ms == 0) {
            fsm->undervoltage_start_ms = in->now_ms;
        }
        uint64_t elapsed = (in->now_ms - fsm->undervoltage_start_ms) / MS_PER_SEC;
        if (elapsed >= fsm->cfg.undervoltage_time_s) {
            fsm->out.state = ELECTRIC_STATE_UNDERVOLTAGE;
            fsm->out.force_safe_off = true;
            fsm->out.safeoff_reason = SAFEOFF_REASON_UNDERVOLTAGE;
            /* @requirement RF-ENERGY-007 ALM-051 = Subtensão de rede (tabela ALM SRS). */
            fsm->out.suggested_alm = ALM_051;
            return;
        }
        fsm->out.state = ELECTRIC_STATE_UNDERVOLTAGE;
    } else {
        fsm->undervoltage_start_ms = 0;
    }

    /* Total current overload with persistence (RF-ENERGY-008): a corrente total do
     * PZEM é a fonte primária da proteção; a soma dos ACS712 é fallback quando o PZEM
     * não fornece leitura válida (<= 0). */
    if (fsm->cfg.total_current_limit_a > 0.0f) {
        float total_current = in->pzem_total_current_a;
        if (total_current <= 0.0f) {
            total_current = 0.0f;
            for (uint8_t i = 0; i < in->plug_count && i < 10; i++) {
                total_current += in->plug_currents_a[i];
            }
        }
        if (total_current > fsm->cfg.total_current_limit_a) {
            if (fsm->total_overcurrent_start_ms == 0) {
                fsm->total_overcurrent_start_ms = in->now_ms;
            }
            uint64_t elapsed = (in->now_ms - fsm->total_overcurrent_start_ms) / MS_PER_SEC;
            if (elapsed >= fsm->cfg.total_current_time_s) {
                fsm->out.state = ELECTRIC_STATE_OVERLOAD;
                fsm->out.force_safe_off = true;
                fsm->out.safeoff_reason = SAFEOFF_REASON_ELECTRIC_TOTAL;
                fsm->out.suggested_alm = ALM_052;
                return;
            }
        } else {
            fsm->total_overcurrent_start_ms = 0;
        }
    }

    /* Total power overload */
    if (in->total_power_w > fsm->cfg.total_power_limit_w) {
        fsm->out.state = ELECTRIC_STATE_OVERLOAD;
        fsm->out.force_safe_off = true;
        fsm->out.safeoff_reason = SAFEOFF_REASON_ELECTRIC_TOTAL;
        fsm->out.suggested_alm = ALM_052;
        return;
    }

    /* Power factor monitoring (RF-ENERGY-009): WARNING-only, sem mudança de estado
     * nem SAFE_OFF. @requirement ALM-053 = Fator de potência baixo (tabela ALM SRS). */
    if (in->pf > 0.01f && in->pf < fsm->cfg.pf_min) {
        if (fsm->pf_low_start_ms == 0) {
            fsm->pf_low_start_ms = in->now_ms;
        }
        uint64_t elapsed = (in->now_ms - fsm->pf_low_start_ms) / MS_PER_SEC;
        if (elapsed >= fsm->cfg.pf_time_s) {
            fsm->out.suggested_alm = ALM_053;
            /* não retorna: PF baixo é apenas advertência, segue avaliação de zona. */
        }
    } else {
        fsm->pf_low_start_ms = 0;
    }

    /* High consumption warning zone (hysteresis-based) */
    if (in->total_power_w > (fsm->cfg.total_power_limit_w - fsm->cfg.hysteresis_w)) {
        fsm->out.state = ELECTRIC_STATE_HIGH_CONSUMPTION;
        return;
    }

    fsm->out.state = ELECTRIC_STATE_NORMAL;
}

const electric_output_t* electric_fsm_get_output(const electric_fsm_t *fsm)
{
    if (!fsm) return NULL;
    return &fsm->out;
}

void electric_fsm_force_safe_off(electric_fsm_t *fsm)
{
    if (!fsm) return;
    fsm->out.force_safe_off = true;
    fsm->out.safeoff_reason = SAFEOFF_REASON_ELECTRIC_TOTAL;
}
