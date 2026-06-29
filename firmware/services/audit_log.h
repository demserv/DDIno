// @requirement RNF-SECURITY-003 Log de auditoria com timestamp, evento, resultado
#ifndef FIRMWARE_SERVICES_AUDIT_LOG_H
#define FIRMWARE_SERVICES_AUDIT_LOG_H

#include "esp_err.h"
#include <stdint.h>

#define AUDIT_LOG_LINE_SIZE 256

typedef enum {
    AUDIT_SYSTEM_STATE,
    AUDIT_CONFIG_CHANGE,
    AUDIT_COMMAND,
    AUDIT_SAFE_OFF,
    AUDIT_EMERGENCY,
    AUDIT_SELF_TEST,
    AUDIT_FEED_MODE,
    AUDIT_LOGIN,
    AUDIT_WIZARD,
    AUDIT_MAINTENANCE,
    AUDIT_FACTORY_RESET
} audit_event_type_t;

esp_err_t audit_log_event(audit_event_type_t type, const char *msg);
esp_err_t audit_log_state_change(const char *from_state, const char *to_state, const char *reason);
esp_err_t audit_log_command(const char *cmd, const char *target, const char *result);

#endif
