// @requirement RF-LOG-ELECTRIC-001 Logs elétricos estruturados (CSV no SD)
#include "electric_log.h"
#include "storage_sd.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

static const char *event_name(electric_log_event_t type)
{
    switch (type) {
        case ELECTRIC_LOG_OVERVOLTAGE:        return "OVERVOLTAGE";
        case ELECTRIC_LOG_UNDERVOLTAGE:       return "UNDERVOLTAGE";
        case ELECTRIC_LOG_OVERPOWER:          return "OVERPOWER";
        case ELECTRIC_LOG_OVERCURRENT_TOTAL:  return "OVERCURRENT_TOTAL";
        case ELECTRIC_LOG_PLUG_OVERCURRENT:   return "PLUG_OVERCURRENT";
        case ELECTRIC_LOG_PLUG_SHORT:         return "PLUG_SHORT";
        case ELECTRIC_LOG_PLUG_BLOCKED:       return "PLUG_BLOCKED";
        case ELECTRIC_LOG_SAFE_OFF:           return "SAFE_OFF";
        case ELECTRIC_LOG_PF_LOW:             return "PF_LOW";
        default:                              return "UNKNOWN";
    }
}

esp_err_t electric_log_init(void)
{
    return ESP_OK;
}

void electric_log_event(electric_log_event_t type, uint8_t plug_id, float value, const char *detail)
{
    char line[256];
    uint64_t now_ms = esp_timer_get_time() / 1000ULL;
    snprintf(line, sizeof(line), "%llu,%s,%u,%.2f,%s",
             (unsigned long long)now_ms,
             event_name(type),
             (unsigned)plug_id,
             (double)value,
             detail ? detail : "");
    storage_sd_write_log(SD_LOG_TYPE_ELECTRIC, line);
}
