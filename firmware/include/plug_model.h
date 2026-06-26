#ifndef FIRMWARE_INCLUDE_PLUG_MODEL_H
#define FIRMWARE_INCLUDE_PLUG_MODEL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PLUG_ID_P01 = 1,
    PLUG_ID_P02,
    PLUG_ID_P03,
    PLUG_ID_P04,
    PLUG_ID_P05,
    PLUG_ID_P06,
    PLUG_ID_P07,
    PLUG_ID_P08,
    PLUG_ID_P09,
    PLUG_ID_P10
} plug_id_t;

typedef enum {
    PLUG_EFFECTIVE_STATE_OFF = 0,
    PLUG_EFFECTIVE_STATE_ON,
    PLUG_EFFECTIVE_STATE_BLOCKED,
    PLUG_EFFECTIVE_STATE_FAULT,
    PLUG_EFFECTIVE_STATE_WAITING,
    PLUG_EFFECTIVE_STATE_UNAVAILABLE
} plug_effective_state_t;

typedef enum {
    PLUG_TYPE_BOMBA = 0,
    PLUG_TYPE_AQUECEDOR,
    PLUG_TYPE_COOLER,
    PLUG_TYPE_FILTRO,
    PLUG_TYPE_AIREADOR,
    PLUG_TYPE_LUZ,
    PLUG_TYPE_OUTRO
} plug_type_t;

typedef enum {
    PLUG_MODE_AUTO = 0,
    PLUG_MODE_MANUAL,
    PLUG_MODE_TIMER,
    PLUG_MODE_DELAY,
    PLUG_MODE_OVERRIDE
} plug_mode_t;

typedef enum {
    PLUG_VISUAL_STATE_ON_NORMAL = 0,
    PLUG_VISUAL_STATE_OFF_NORMAL,
    PLUG_VISUAL_STATE_OFF_FAULT,
    PLUG_VISUAL_STATE_OFF_WAITING,
    PLUG_VISUAL_STATE_BLOCKED
} plug_visual_state_t;

typedef struct {
    plug_id_t               id;
    char                    name[24];
    plug_type_t             type;
    plug_mode_t             mode;
    bool                    command_allowed;
    bool                    blocked_by_safe_state;
    bool                    monitor_only_blocked;
    plug_effective_state_t  effective_state;
    plug_visual_state_t     visual_state;
    bool                    is_critical;
    bool                    blocked;
    bool                    bypass_detected;
    float                   current_a;
    float                   power_w;
    float                   rated_power_w;
    float                   current_limit_a;
    float                   min_operating_voltage_v;
    float                   fator_curto;
    uint32_t                tempo_deteccao_curto_ms;
    uint32_t                min_on_time_s;
    uint32_t                min_off_time_s;
    uint32_t                overcurrent_count;
    uint32_t                time_on_today_s;
    uint32_t                timer_on_s;
    uint32_t                timer_off_s;
    uint32_t                delay_seconds;
    float                   energy_wh_today;
    float                   energy_wh_24h;
    float                   energy_wh_total;
    float                   max_energy_wh_day;
    bool                    maintenance_exempt;
    uint8_t                 role_override_source;
} plug_model_t;

#define PLUG_COUNT_TOTAL (10U)

#endif
