// @requirement RF-GLOBAL-001 Tipos de dados compartilhados da HMI
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    UI_SYSTEM_NORMAL = 0,
    UI_SYSTEM_DEGRADED,
    UI_SYSTEM_SAFE_OFF,
    UI_SYSTEM_EMERGENCY
} ui_system_state_t;

typedef enum {
    UI_SEVERITY_INFO = 0,
    UI_SEVERITY_WARNING,
    UI_SEVERITY_HIGH,
    UI_SEVERITY_CRITICAL
} ui_alert_severity_t;

typedef enum {
    UI_PLUG_OFF = 0,
    UI_PLUG_ON,
    UI_PLUG_BLOCKED,
    UI_PLUG_ERROR
} ui_plug_state_t;

typedef enum {
    UI_THERMAL_IDLE = 0,
    UI_THERMAL_HEATING,
    UI_THERMAL_COOLING,
    UI_THERMAL_ALERT,
    UI_THERMAL_CRITICAL
} ui_thermal_state_t;

typedef enum {
    UI_ATO_IDLE = 0,
    UI_ATO_REFILLING,
    UI_ATO_BLOCKED,
    UI_ATO_ERROR
} ui_ato_state_t;

typedef enum {
    UI_HEALTH_UNKNOWN = 0,
    UI_HEALTH_OK,
    UI_HEALTH_DEGRADED,
    UI_HEALTH_FAILED
} ui_health_state_t;
