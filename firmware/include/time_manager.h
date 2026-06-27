// @requirement RF-TIME-001..010 Gerenciamento de tempo: RTC, NTP, timezone
#ifndef FIRMWARE_INCLUDE_TIME_MANAGER_H
#define FIRMWARE_INCLUDE_TIME_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TIME_SOURCE_NONE = 0,
    TIME_SOURCE_RTC,
    TIME_SOURCE_NTP,
    TIME_SOURCE_USER
} time_source_t;

esp_err_t time_manager_init(void);
esp_err_t time_get(time_t *out_unix_ts);
esp_err_t time_set(time_t unix_ts, time_source_t src);
esp_err_t time_get_source(time_source_t *out);
bool time_is_valid(void);
esp_err_t time_sync_ntp_async(const char *ntp_server);
esp_err_t time_set_timezone(const char *tz);
bool time_is_ntp_synced(void);

#ifdef __cplusplus
}
#endif

#endif
