// @requirement RF-LOG-ELECTRIC-001 Logs elétricos estruturados
#ifndef FIRMWARE_SERVICES_ELECTRIC_LOG_H
#define FIRMWARE_SERVICES_ELECTRIC_LOG_H

#include "esp_err.h"
#include <stdint.h>

typedef enum {
    ELECTRIC_LOG_OVERVOLTAGE = 0,
    ELECTRIC_LOG_UNDERVOLTAGE,
    ELECTRIC_LOG_OVERPOWER,
    ELECTRIC_LOG_OVERCURRENT_TOTAL,
    ELECTRIC_LOG_PLUG_OVERCURRENT,
    ELECTRIC_LOG_PLUG_SHORT,
    ELECTRIC_LOG_PLUG_BLOCKED,
    ELECTRIC_LOG_SAFE_OFF,
    ELECTRIC_LOG_PF_LOW
} electric_log_event_t;

esp_err_t electric_log_init(void);
void electric_log_event(electric_log_event_t type, uint8_t plug_id, float value, const char *detail);

#endif
