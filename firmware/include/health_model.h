// @requirement RF-WEB-005 Modelo de health consolidado (heap, SD, WiFi, uptime)
#ifndef FIRMWARE_INCLUDE_HEALTH_MODEL_H
#define FIRMWARE_INCLUDE_HEALTH_MODEL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool     electric_ok;
    bool     temp_ok;
    bool     ato_ok;
    bool     pzem_ok;
    bool     sd_ok;
    bool     wifi_ok;
    bool     ui_ok;
    bool     selftest_passed;
    uint64_t last_health_check_timestamp;
} health_model_t;

#endif
