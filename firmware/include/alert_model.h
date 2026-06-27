// @requirement RF-ALERT-001 Modelo canônico de alerta com severity, category, action_hint
#ifndef FIRMWARE_INCLUDE_ALERT_MODEL_H
#define FIRMWARE_INCLUDE_ALERT_MODEL_H

#include <stdbool.h>
#include <stdint.h>
#include "alm_ids.h"

typedef enum {
    ALERT_SEVERITY_INFO = 0,
    ALERT_SEVERITY_WARNING,
    ALERT_SEVERITY_HIGH,
    ALERT_SEVERITY_CRITICAL
} alert_severity_t;

typedef enum {
    ALERT_CATEGORY_PROCESS = 0,
    ALERT_CATEGORY_SYSTEM,
    ALERT_CATEGORY_SECURITY
} alert_category_t;

typedef enum {
    ALERT_SOURCE_LOCAL = 0,
    ALERT_SOURCE_WEB_API,
    ALERT_SOURCE_SYSTEM
} alert_source_t;

typedef struct {
    alm_id_t          alm_id;
    alert_severity_t  severity;
    alert_category_t  category;
    bool              active;
    bool              acked;
    bool              ack_req;
    uint64_t          first_seen_ts;
    uint64_t          last_seen_ts;
    uint64_t          ack_timestamp;
    char              message[128];
    float             value;
    bool              auto_clear;
    char              action_hint[64];
    uint64_t          silenced_until;
    alert_source_t    source;
    uint16_t          related_plug_id;
    uint16_t          related_sensor;
    bool              state_associated;
} alert_model_t;

#endif
