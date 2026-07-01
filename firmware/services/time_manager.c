// @requirement RF-TIME-001..010 Gerenciamento de tempo
// @requirement RF-TIME-003 Sincronizacao NTP funcional com fallback RTC
#include "time_manager.h"

#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"

#include "driver_ds3231.h"

static const char *TAG = "time_mgr";
static time_t s_current = 0;
static time_source_t s_source = TIME_SOURCE_NONE;
static bool s_valid = false;
static char s_timezone[64] = "America/Sao_Paulo";
static bool s_ntp_synced = false;
static char s_ntp_server[128] = "pool.ntp.org";

static void sntp_callback(struct timeval *tv)
{
    if (tv->tv_sec > 100000) {
        s_current = tv->tv_sec;
        s_source = TIME_SOURCE_NTP;
        s_valid = true;
        s_ntp_synced = true;
        struct tm ti;
        localtime_r(&s_current, &ti);
        ds3231_time_t ds;
        ds.second = ti.tm_sec;
        ds.minute = ti.tm_min;
        ds.hour = ti.tm_hour;
        ds.day = ti.tm_mday;
        ds.month = ti.tm_mon + 1;
        ds.year = ti.tm_year + 1900;
        ds3231_set_time(&ds);
        ESP_LOGI(TAG, "NTP sync OK: %04d-%02d-%02dT%02d:%02d:%02d",
                 ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second);
    }
}

esp_err_t time_manager_init(void)
{
    s_current = 0;
    s_source = TIME_SOURCE_NONE;
    s_valid = false;
    s_ntp_synced = false;
    ds3231_time_t rtc;
    if (ds3231_get_time(&rtc) == ESP_OK && rtc.year >= 2024) {
        struct tm tm_rtc;
        memset(&tm_rtc, 0, sizeof(tm_rtc));
        tm_rtc.tm_sec = rtc.second;
        tm_rtc.tm_min = rtc.minute;
        tm_rtc.tm_hour = rtc.hour;
        tm_rtc.tm_mday = rtc.day;
        tm_rtc.tm_mon = rtc.month - 1;
        tm_rtc.tm_year = rtc.year - 1900;
        s_current = mktime(&tm_rtc);
        s_source = TIME_SOURCE_RTC;
        s_valid = true;
        ESP_LOGI(TAG, "RTC time restored: %04d-%02d-%02dT%02d:%02d:%02d",
                 rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);
    } else {
        ESP_LOGW(TAG, "RTC not available, time will be set via NTP later");
    }
    ESP_LOGI(TAG, "Time manager initialized");
    return ESP_OK;
}

esp_err_t time_get(time_t *out_unix_ts)
{
    if (!out_unix_ts) return ESP_ERR_INVALID_ARG;
    *out_unix_ts = s_current;
    return s_valid ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t time_set(time_t unix_ts, time_source_t src)
{
    s_current = unix_ts;
    s_source = src;
    s_valid = true;
    struct tm ti;
    localtime_r(&s_current, &ti);
    ds3231_time_t ds;
    ds.second = ti.tm_sec;
    ds.minute = ti.tm_min;
    ds.hour = ti.tm_hour;
    ds.day = ti.tm_mday;
    ds.month = ti.tm_mon + 1;
    ds.year = ti.tm_year + 1900;
    ds3231_set_time(&ds);
    ESP_LOGI(TAG, "Time set from %d: %04d-%02d-%02dT%02d:%02d:%02d",
             (int)src, ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second);
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
    strncpy(s_ntp_server, ntp_server, sizeof(s_ntp_server) - 1);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, s_ntp_server);
    sntp_set_time_sync_notification_cb(sntp_callback);
    sntp_init();
    ESP_LOGI(TAG, "NTP sync initiated from %s", s_ntp_server);
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

bool time_is_ntp_synced(void)
{
    return s_ntp_synced;
}

