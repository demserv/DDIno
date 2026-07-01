// @requirement RF-THERMAL-001 Leitura contínua com validação robusta (CRC + 85°C rejection)
// @requirement RF-THERMAL-002 Sensor fail → DEGRADED (alerta critico com ACK do usuario)
// @requirement RF-THERMAL-008 Prioridade heater sobre cooler
#ifndef FIRMWARE_FSM_THERMAL_FSM_H
#define FIRMWARE_FSM_THERMAL_FSM_H

#include <stdbool.h>
#include <stdint.h>
#include "system_types.h"
#include "alm_ids.h"

typedef enum {
    THERMAL_STATE_NORMAL = 0,
    THERMAL_STATE_ALERT,
    THERMAL_STATE_CRITICAL,
    THERMAL_STATE_EXTREME,
    THERMAL_STATE_SENSOR_FAIL
} thermal_state_t;

typedef struct {
    float temp_normal_c;
    float temp_critical_c;
    float temp_extreme_c;
    float temp_min_c;
    float temp_max_c;
    float hysteresis_c;
    bool  extreme_enabled;
} thermal_params_t;

typedef struct {
    bool     sample_valid;
    float    temp_c;
    uint64_t now_ms;
} thermal_input_t;

typedef struct {
    thermal_state_t state;
    bool request_heater_on;
    bool request_cooler_on;
    bool force_safe_off;
    bool force_emergency;
    bool sensor_fault;
    safeoff_reason_t safeoff_reason;
    uint16_t suggested_alm;
    float trend_c_per_min;  /* positive = warming, negative = cooling, 0 = stable */
} thermal_output_t;

typedef struct {
    thermal_params_t cfg;
    thermal_output_t out;
    bool over_temp_latched;
    float    last_valid_temp_c;
    uint64_t last_valid_at_ms;
} thermal_fsm_t;

void thermal_fsm_init(thermal_fsm_t *fsm, const thermal_params_t *cfg);
void thermal_fsm_set_config(thermal_fsm_t *fsm, const thermal_params_t *cfg);
void thermal_fsm_update(thermal_fsm_t *fsm, const thermal_input_t *in);
void thermal_fsm_clear_over_temp_latch(thermal_fsm_t *fsm);
const thermal_output_t* thermal_fsm_get_output(const thermal_fsm_t *fsm);

/* @requirement RF-THERMAL-001 cache de última leitura válida */
bool thermal_fsm_get_last_valid_temp(const thermal_fsm_t *f, float *out_c);

#endif
