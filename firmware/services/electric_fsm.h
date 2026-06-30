// @requirement RF-FSM-ELECTRIC-001 FSM de proteção elétrica com 8 estados
// @requirement RF-ENERGY-007 a RF-ENERGY-009 Proteções elétricas via FSM
#ifndef FIRMWARE_SERVICES_ELECTRIC_FSM_H
#define FIRMWARE_SERVICES_ELECTRIC_FSM_H

#include <stdbool.h>
#include <stdint.h>
#include "system_types.h"
#include "alm_ids.h"

typedef enum {
    ELECTRIC_STATE_NORMAL = 0,
    ELECTRIC_STATE_HIGH_CONSUMPTION,
    ELECTRIC_STATE_OVERLOAD,
    ELECTRIC_STATE_OVERVOLTAGE,
    ELECTRIC_STATE_UNDERVOLTAGE,
    ELECTRIC_STATE_SHORT_CIRCUIT,
    ELECTRIC_STATE_SENSOR_FAIL,
    ELECTRIC_STATE_NORMALIZING
} electric_state_t;

typedef struct {
    float total_power_limit_w;
    float per_plug_current_limit_a;
    float hysteresis_w;
    float overvoltage_limit_v;
    float undervoltage_limit_v;
    uint32_t overvoltage_time_s;
    uint32_t undervoltage_time_s;
    float total_current_limit_a;
    uint32_t total_current_time_s;
    float pf_min;
    uint32_t pf_time_s;
    float fator_curto;
    uint32_t tempo_deteccao_curto_ms;
} electric_params_t;

typedef struct {
    bool    sample_valid;
    float   total_power_w;
    float   total_energy_wh;
    float   plug_currents_a[10];
    uint8_t plug_count;
    /* @requirement RF-ENERGY-008 Corrente total medida pelo PZEM (fonte primária da
     * proteção total). Quando <= 0, a FSM faz fallback para a soma dos ACS712. */
    float   pzem_total_current_a;
    float   voltage_v;
    float   frequency_hz;
    float   pf;
    uint64_t now_ms;
} electric_input_t;

typedef struct {
    electric_state_t state;
    bool force_safe_off;
    safeoff_reason_t safeoff_reason;
    uint16_t suggested_alm;
    float measured_total_power_w;
    uint8_t shorted_plug_id;
} electric_output_t;

typedef struct {
    electric_params_t cfg;
    electric_output_t out;
    uint32_t plug_overcurrent_count[10];
    uint64_t plug_short_start_ms[10];
    uint64_t overvoltage_start_ms;
    uint64_t undervoltage_start_ms;
    uint64_t total_overcurrent_start_ms;
    uint64_t pf_low_start_ms;
} electric_fsm_t;

void electric_fsm_init(electric_fsm_t *fsm, const electric_params_t *cfg);
void electric_fsm_update(electric_fsm_t *fsm, const electric_input_t *in);
const electric_output_t* electric_fsm_get_output(const electric_fsm_t *fsm);
/* @requirement RF-FSM-ELECTRIC-001 Força SAFE_OFF por causa elétrica total. */
void electric_fsm_force_safe_off(electric_fsm_t *fsm);

#endif
