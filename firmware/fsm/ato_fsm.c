// @requirement RF-ATO-001 Leitura de nível com debounce ADC temporal (3 amostras)
// @requirement RF-ATO-DIGITAL-001 ATO digital ON/OFF com histerese ADC
// @requirement RF-ATO-002 FSM de proteção ATO com seis estados
// @requirement RF-ATO-004 Detecção de padrão anormal de refill
// @requirement RF-ATO-005 Detecção de reservatório vazio ou bloqueio
// @requirement RF-FSM-ATO-001 Impacto dos estados ATO no estado global
#include "fsm/ato_fsm.h"
#include "hardware_config.h"
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
    fsm->debounce_required = HW_ATO_DEBOUNCE_COUNT;
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
    if (diff < HW_ATO_ADC_HYSTERESIS) {
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

        /* ALM-027: marca o instante em que o nivel entrou abaixo de LOW. */
        if (fsm->low_since_ms == 0) {
            fsm->low_since_ms = in->now_ms;
        }

        if (fsm->refill_started_ms == 0) {
            /* Novo ciclo de refill: registra inicio, nivel base e contabiliza frequencia. */
            fsm->refill_started_ms = in->now_ms;
            fsm->refill_start_level = level;

            /* @requirement RF-ATO-004 ALM-038: frequencia anormal de refill na janela. */
            if (fsm->cycles_window_start_ms == 0 ||
                (in->now_ms - fsm->cycles_window_start_ms) >
                    ((uint64_t)HW_ATO_REFILL_FREQ_WINDOW_S * MS_PER_SEC)) {
                fsm->cycles_window_start_ms = in->now_ms;
                fsm->refill_cycles = 0;
            }
            fsm->refill_cycles++;
            fsm->out.suggested_alm =
                (fsm->refill_cycles > HW_ATO_REFILL_FREQ_MAX) ? ALM_038 : ALM_018;
            return;
        }

        const uint64_t elapsed_ms = in->now_ms - fsm->refill_started_ms;

        /* Timeout total de refill -> BLOCKED + SAFE_OFF (politica consolidada). */
        if (elapsed_ms > ((uint64_t)fsm->cfg.refill_timeout_s * MS_PER_SEC)) {
            fsm->blocked_latched = true;
            fsm->out.state = ATO_STATE_BLOCKED;
            fsm->out.pump_request_on = false;
            fsm->out.force_safe_off = true;
            fsm->out.safeoff_reason = SAFEOFF_REASON_ATO_OVERFLOW;
            fsm->out.suggested_alm = ALM_019;
            return;
        }

        /* @requirement RF-ATO-005 ALM-039: bomba ativa por tempo sem subida
         * suficiente do nivel => reservatorio vazio/bloqueio. Reage como
         * ERROR/DEGRADED e para a bomba (evita funcionamento a seco). A escalada
         * para BLOCKED/SAFE_OFF fica a cargo do timeout total acima. */
        if (elapsed_ms >= ((uint64_t)HW_ATO_EMPTY_DETECT_S * MS_PER_SEC) &&
            (level - fsm->refill_start_level) < (int32_t)HW_ATO_EMPTY_MIN_RISE_ADC) {
            fsm->out.state = ATO_STATE_ERROR;
            fsm->out.pump_request_on = false;
            fsm->out.suggested_alm = ALM_039;
            return;
        }

        /* @requirement ALM-027: nivel permanece abaixo de LOW por tempo prolongado (DEGRADED). */
        if ((in->now_ms - fsm->low_since_ms) >=
                ((uint64_t)HW_ATO_PERSIST_LOW_S * MS_PER_SEC)) {
            fsm->out.suggested_alm = ALM_027;
        }
        return;
    }

    /* Nivel normalizou: encerra refill e condicao de nivel baixo. */
    fsm->refill_started_ms = 0;
    fsm->low_since_ms = 0;
    fsm->out.state = ATO_STATE_NORMAL;
    fsm->debounce_count = 0;
}

const ato_output_t* ato_fsm_get_output(const ato_fsm_t *fsm)
{
    if (!fsm) return NULL;
    return &fsm->out;
}
