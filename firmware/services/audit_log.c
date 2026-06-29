// @requirement RNF-SECURITY-003 Logs de auditoria de segurança
#include "services/audit_log.h"

#include <stdio.h>
#include <string.h>

#include "esp_timer.h"

#include "services/storage_sd.h"

static const char *event_type_str(audit_event_type_t type)
{
    switch (type) {
        case AUDIT_SYSTEM_STATE: return "STATE";
        case AUDIT_CONFIG_CHANGE: return "CONFIG";
        case AUDIT_COMMAND:       return "CMD";
        case AUDIT_SAFE_OFF:      return "SAFEOFF";
        case AUDIT_EMERGENCY:     return "EMERG";
        case AUDIT_SELF_TEST:     return "SELFTEST";
        case AUDIT_FEED_MODE:     return "FEED";
        case AUDIT_LOGIN:         return "LOGIN";
        case AUDIT_WIZARD:        return "WIZARD";
        case AUDIT_MAINTENANCE:   return "MAINT";
        case AUDIT_FACTORY_RESET: return "FACTORY_RESET";
        default:                  return "UNKNOWN";
    }
}

esp_err_t audit_log_event(audit_event_type_t type, const char *msg)
{
    if (!msg) return ESP_ERR_INVALID_ARG;
    if (!storage_sd_is_mounted()) return ESP_ERR_INVALID_STATE;

    uint64_t now_us = esp_timer_get_time();
    char line[256];
    snprintf(line, sizeof(line), "[%llu] [%s] %s",
             (unsigned long long)(now_us / 1000000ULL),
             event_type_str(type), msg);
    return storage_sd_write_log(SD_LOG_TYPE_AUDIT, line);
}

esp_err_t audit_log_state_change(const char *from_state, const char *to_state, const char *reason)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "STATE: %s -> %s | reason=%s",
             from_state ? from_state : "?", to_state ? to_state : "?",
             reason ? reason : "none");
    return audit_log_event(AUDIT_SYSTEM_STATE, buf);
}

esp_err_t audit_log_command(const char *cmd, const char *target, const char *result)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "CMD: %s target=%s result=%s",
             cmd ? cmd : "?", target ? target : "?", result ? result : "?");
    return audit_log_event(AUDIT_COMMAND, buf);
}

