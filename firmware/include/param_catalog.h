// @requirement RF-UI-WIZARD-001..005 Wizard step persistence
#ifndef FIRMWARE_INCLUDE_PARAM_CATALOG_H
#define FIRMWARE_INCLUDE_PARAM_CATALOG_H

#include <stdbool.h>
#include <stdint.h>

#define PARAM_CATALOG_VERSION "1.0.0"

typedef struct {
    float temp_normal_c;
    float temp_critical_c;
    float temp_extreme_c;
    float temp_min_c;
    float temp_max_c;
    float hysteresis_c;
    bool  extreme_enabled;
} thermal_params_storage_t;

typedef struct {
    bool     enabled;
    int32_t  low_level_adc;
    int32_t  high_level_adc;
    int32_t  overflow_margin_adc;
    uint32_t refill_timeout_s;
} ato_params_storage_t;

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
} electric_params_storage_t;

typedef struct {
    uint32_t min_on_time_s;
    uint32_t min_off_time_s;
} plug_limits_storage_t;

typedef struct {
    uint32_t tempo_espera_religamento_s;
    uint32_t intervalo_religamento_s;
    uint32_t tempo_monitoramento_pos_relig_s;
} restart_params_storage_t;

typedef struct {
    uint32_t feed_duration_min;
    uint32_t feed_cooldown_min;
} feed_params_storage_t;

typedef struct {
    uint32_t session_timeout_min;
    uint32_t max_login_attempts;
    uint32_t login_block_duration_min;
    bool     read_requires_auth;
} security_params_storage_t;

typedef struct {
    uint32_t tempo_min_estabilizacao_s;
    uint32_t janela_flap_s;
    uint32_t max_transicoes_flap;
    uint32_t cooldown_reentrada_s;
} antiflap_params_storage_t;

typedef struct {
    uint32_t selftest_timeout_ms;
} selftest_params_storage_t;

typedef struct {
    bool     wizard_completed;
    uint16_t mains_voltage;
    bool     monitor_only_mode;
    bool     maintenance_mode;
    uint8_t  wizard_step;
} system_params_storage_t;

#define PARAM_THERMAL_DEFAULT_TEMP_NORMAL_C      25.0f
#define PARAM_THERMAL_DEFAULT_TEMP_CRITICAL_C    30.0f
#define PARAM_THERMAL_DEFAULT_TEMP_EXTREME_C     35.0f
#define PARAM_THERMAL_DEFAULT_TEMP_MIN_C         18.0f
#define PARAM_THERMAL_DEFAULT_TEMP_MAX_C         35.0f
#define PARAM_THERMAL_DEFAULT_HYSTERESIS_C       1.0f
#define PARAM_THERMAL_DEFAULT_EXTREME_ENABLED    true

#define PARAM_ATO_DEFAULT_ENABLED                true
#define PARAM_ATO_DEFAULT_LOW_ADC                500
#define PARAM_ATO_DEFAULT_HIGH_ADC               2500
#define PARAM_ATO_DEFAULT_OVERFLOW_ADC           200
#define PARAM_ATO_DEFAULT_REFILL_TIMEOUT_S       120

#define PARAM_ELECTRIC_DEFAULT_TOTAL_POWER_W     1200.0f
#define PARAM_ELECTRIC_DEFAULT_PER_PLUG_CURRENT  10.0f
#define PARAM_ELECTRIC_DEFAULT_HYSTERESIS_W      50.0f
#define PARAM_ELECTRIC_DEFAULT_OVERVOLTAGE_127V  140.0f
#define PARAM_ELECTRIC_DEFAULT_OVERVOLTAGE_220V  250.0f
#define PARAM_ELECTRIC_DEFAULT_UNDERVOLTAGE_127V 105.0f
#define PARAM_ELECTRIC_DEFAULT_UNDERVOLTAGE_220V 190.0f
#define PARAM_ELECTRIC_DEFAULT_OV_TIME_S         5
#define PARAM_ELECTRIC_DEFAULT_UV_TIME_S         5
#define PARAM_ELECTRIC_DEFAULT_TOTAL_CURRENT_A   15.0f
#define PARAM_ELECTRIC_DEFAULT_TOTAL_CURRENT_TIME_S 5
#define PARAM_ELECTRIC_DEFAULT_PF_MIN            0.5f
#define PARAM_ELECTRIC_DEFAULT_PF_TIME_S         10
#define PARAM_ELECTRIC_DEFAULT_FATOR_CURTO       2.0f
#define PARAM_ELECTRIC_DEFAULT_TEMP_CURTO_MS     500

#define PARAM_PLUG_DEFAULT_MIN_ON_S              30
#define PARAM_PLUG_DEFAULT_MIN_OFF_S             30

#define PARAM_RESTART_DEFAULT_ESPERA_S           30
#define PARAM_RESTART_DEFAULT_INTERVALO_S        5
#define PARAM_RESTART_DEFAULT_MONITOR_S          10

#define PARAM_FEED_DEFAULT_DURATION_MIN          10

#define PARAM_SECURITY_DEFAULT_SESSION_TIMEOUT   60
#define PARAM_SECURITY_DEFAULT_MAX_LOGIN         5
#define PARAM_SECURITY_DEFAULT_BLOCK_DURATION    15

#define PARAM_ANTIFLAP_DEFAULT_ESTABILIZACAO_S   10
#define PARAM_ANTIFLAP_DEFAULT_JANELA_S          60
#define PARAM_ANTIFLAP_DEFAULT_MAX_TRANSICOES    3
#define PARAM_ANTIFLAP_DEFAULT_COOLDOWN_S        300

#define PARAM_SELFTEST_DEFAULT_TIMEOUT_MS        10000

#define PARAM_SYSTEM_DEFAULT_MAINS_VOLTAGE       127
#define PARAM_SYSTEM_DEFAULT_MONITOR_ONLY        false
#define PARAM_SYSTEM_DEFAULT_MAINTENANCE_MODE    false

typedef struct {
    float acs712_zero_offset_mv[10];
    int32_t ato_zero_offset_adc;
    float temp_offset_c;
} calibration_params_storage_t;

#define PARAM_CALIB_DEFAULT_ATO_ZERO_ADC     0
#define PARAM_CALIB_DEFAULT_TEMP_OFFSET_C    0.0f

#endif
