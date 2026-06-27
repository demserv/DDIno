// @requirement RF-TIME-001..010 Gerenciamento de tempo
#include "time_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "time_mgr";
static time_t s_current = 0;
static time_source_t s_source = TIME_SOURCE_NONE;
static bool s_valid = false;
static char s_timezone[64] = "America/Sao_Paulo";

esp_err_t time_manager_init(void)
{
    s_current = 0;
    s_source = TIME_SOURCE_NONE;
    s_valid = false;
    ESP_LOGI(TAG, "Time manager initialized");
    return ESP_OK;
}

esp_err_t time_get(time_t *out_unix_ts)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out_unix_ts = s_current;
    return s_valid ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t time_set(time_t unix_ts, time_source_t src)
{
    s_current = unix_ts;
    s_source = src;
    s_valid = true;
    ESP_LOGI(TAG, "Time set from %d", (int)src);
    return ESP_OK;
}

esp_err_t time_get_source(time_source_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_source;
    return ESP_OK;
}

bool time_is_valid(void) { return s_valid; }

esp_err_t time_sync_ntp_async(const char *ntp_server)
{
    if (!ntp_server) ntp_server = "pool.ntp.org";
    ESP_LOGI(TAG, "NTP sync requested from %s", ntp_server);
    return ESP_OK;
}

esp_err_t time_set_timezone(const char *tz)
{
    if (!tz) return ESP_ERR_INVALID_ARG;
    strncpy(s_timezone, tz, sizeof(s_timezone) - 1);
    setenv("TZ", s_timezone, 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set to %s", s_timezone);
    return ESP_OK;
}
