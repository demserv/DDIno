/* @requirement RF-WEB-001 a RF-WEB-008 RF-API-SCHEMA-001 a 003 */
// @requirement RF-WEB-001 a RF-WEB-008 (API REST)
// @requirement RNF-CALIB-001 Calibração assistida via API
#include "api_rest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "api_auth.h"
#include "api_rate_limit.h"
#include "circuit_breaker.h"
#include "wifi_ctl.h"
#include "services/self_test.h"
#include "services/storage_sd.h"
#include "driver_acs712.h"
#include "driver_ds18b20.h"
#include "driver_ds3231.h"
#include "driver_mcp3208.h"
#include "driver_pzem.h"
#include "driver_relay.h"
#include "fsm/feed_fsm.h"
#include "fsm/restart_fsm.h"
#include "global_state.h"
#include "safety_controller.h"
#include "health_matrix.h"
#include "pin_map.h"
#include "services/alert_manager.h"
#include "alert_model.h"
#include "services/audit_log.h"
#include "services/command_validator.h"
#include "services/config_manager.h"
#include "services/config_export.h"
#include "services/plug_manager.h"
#include "services/plug_preset_catalog.h"
#include "services/storage_facade.h"
#include "services/wdt_stats.h"
#include "services/relay_safety_service.h"
#include "services/reset_handler.h"
#include "services/safe_state_ack.h"
#include "services/auth_recovery_sd.h"
#include "system_types.h"
#include <time.h>

static const char *TAG = "api_rest";

extern global_state_t g_gs;
extern pzem_data_t g_pzem;
extern bool g_feed_request;
extern restart_fsm_t g_restart_fsm;

static httpd_handle_t s_server = NULL;

#define ERR_AUTH_REQUIRED    1001
#define ERR_INVALID_TOKEN    1002
#define ERR_RATE_LIMITED     1003
#define ERR_VALIDATION_ERROR 2001
#define ERR_SAFE_MODE_ACTIVE 2003
#define ERR_NOT_FOUND        2002
#define ERR_INTERNAL         5001

static char *read_body(httpd_req_t *req)
{
    if (req->content_len <= 0) return NULL;
    char *buf = malloc(req->content_len + 1);
    if (!buf) return NULL;
    int offset = 0;
    while (offset < req->content_len) {
        int ret = httpd_req_recv(req, buf + offset, req->content_len - offset);
        if (ret <= 0) { free(buf); return NULL; }
        offset += ret;
    }
    buf[offset] = '\0';
    return buf;
}

static esp_err_t send_json_resp(httpd_req_t *req, cJSON *json, const char *status_str)
{
    char *str = cJSON_PrintUnformatted(json);
    if (!str) {
        cJSON_Delete(json);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\":\"internal\"}");
        return ESP_FAIL;
    }
    httpd_resp_set_status(req, status_str);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    esp_err_t ret = httpd_resp_sendstr(req, str);
    free(str);
    cJSON_Delete(json);
    return ret;
}

static esp_err_t send_error(httpd_req_t *req, const char *err, int code, int http_code, const char *http_str)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "error", err ? err : "unknown");
    cJSON_AddNumberToObject(json, "code", code);
    return send_json_resp(req, json, http_str);
}

static bool auth_middleware(httpd_req_t *req)
{
    size_t buf_len = httpd_req_get_hdr_value_len(req, "X-Auth-Token") + 1;
    if (buf_len <= 1) return false;
    char *token = malloc(buf_len);
    if (!token) return false;
    if (httpd_req_get_hdr_value_str(req, "X-Auth-Token", token, buf_len) != ESP_OK) {
        free(token);
        return false;
    }
    bool valid = api_auth_validate(token);
    free(token);
    return valid;
}

static uint32_t client_ip_of(httpd_req_t *req)
{
    char ip_str[16] = {0};
    if (httpd_req_get_hdr_value_str(req, "X-Forwarded-For", ip_str, sizeof(ip_str)) != ESP_OK) {
        httpd_req_get_hdr_value_str(req, "X-Real-IP", ip_str, sizeof(ip_str));
    }
    uint32_t ip = 0;
    int parts[4] = {0};
    if (sscanf(ip_str, "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) == 4) {
        ip = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
    }
    return ip;
}

static bool rate_limit_middleware(httpd_req_t *req)
{
    return rate_limit_check(client_ip_of(req));
}

static bool is_auth_path(const char *uri)
{
    return (strcmp(uri, "/api/v1/auth/login") == 0) ||
           (strcmp(uri, "/api/v1/auth/logout") == 0) ||
           (strcmp(uri, "/api/v1/auth/password") == 0) ||
           (strcmp(uri, "/api/v1/wizard") == 0);
}

static esp_err_t auth_guard(httpd_req_t *req)
{
    if (is_auth_path(req->uri)) return ESP_OK;
    if (!auth_middleware(req)) {
        send_error(req, "auth_required", ERR_AUTH_REQUIRED, 401, "401 Unauthorized");
        return ESP_FAIL;
    }
    if (!rate_limit_middleware(req)) {
        send_error(req, "rate_limited", ERR_RATE_LIMITED, 429, "429 Too Many Requests");
        return ESP_FAIL;
    }
    return ESP_OK;
}

#define AUTH_GUARD() do { if (auth_guard(req) != ESP_OK) return ESP_FAIL; } while(0)

static esp_err_t status_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "system_state", g_gs.system_state);
    cJSON_AddBoolToObject(json, "monitor_only_mode", g_gs.monitor_only_mode);
    cJSON_AddBoolToObject(json, "maintenance_mode", g_gs.maintenance_mode);
    cJSON_AddBoolToObject(json, "observation_mode", g_gs.observation_mode);
    cJSON_AddNumberToObject(json, "active_alerts", g_gs.active_alerts_count);
    cJSON_AddNumberToObject(json, "critical_alerts", g_gs.critical_alerts_count);
    cJSON_AddNumberToObject(json, "uptime_s", (double)g_gs.uptime_s);
    cJSON_AddStringToObject(json, "fw_version", g_gs.fw_version);
    cJSON_AddNumberToObject(json, "safeoff_reason", g_gs.safeoff_reason);
    cJSON_AddStringToObject(json, "safeoff_entered_at", g_gs.safeoff_entered_at);
    cJSON_AddStringToObject(json, "safeoff_source_alm", g_gs.safeoff_source_alm);
    cJSON_AddBoolToObject(json, "wizard_completed", g_gs.wizard_completed);
    cJSON_AddBoolToObject(json, "temp_ok", g_gs.temp_ok);
    cJSON_AddBoolToObject(json, "ato_ok", g_gs.ato_ok);
    cJSON_AddBoolToObject(json, "pzem_ok", g_gs.pzem_ok);
    cJSON_AddBoolToObject(json, "sd_ok", g_gs.sd_ok);
    cJSON_AddBoolToObject(json, "wifi_ok", g_gs.wifi_ok);
    cJSON_AddBoolToObject(json, "ui_ok", g_gs.ui_ok);
    cJSON_AddBoolToObject(json, "selftest_passed", g_gs.selftest_passed);
    cJSON_AddBoolToObject(json, "hw_ok", g_gs.hw_ok);
    cJSON_AddBoolToObject(json, "feed_active", g_gs.feed_active);
    cJSON_AddNumberToObject(json, "feed_remaining_s", g_gs.feed_remaining_s);
    cJSON_AddNumberToObject(json, "temp_filtered_c", g_gs.temp_filtered_c);
    cJSON_AddStringToObject(json, "srs_version", g_gs.srs_version);
    cJSON_AddNumberToObject(json, "active_alm_categories", (double)g_gs.active_alm_categories);
    cJSON_AddNumberToObject(json, "last_reset_reason", g_gs.last_reset_reason);
    cJSON_AddBoolToObject(json, "restart_in_progress", g_gs.restart_in_progress);
    httpd_resp_set_hdr(req, "X-Deprecated", "use /api/v1/state");
    return send_json_resp(req, json, "200 OK");
}

/* @requirement RF-WEB-002 GET /api/v1/state (TC-WEB-001) */
static const char *time_source_str(time_source_t s)
{
    switch ((int)s) {
        case TIME_SOURCE_NONE: return "NONE";
        case TIME_SOURCE_RTC:  return "RTC";
        case TIME_SOURCE_NTP:  return "NTP";
        default:               return "USER";
    }
}

static const char *reset_reason_str(uint32_t r)
{
    switch (r) {
        case 1:  return "POWERON";
        case 2:  return "EXT";
        case 3:  return "SW";
        case 4:  return "PANIC";
        case 5:  return "INT_WDT";
        case 6:  return "TASK_WDT";
        case 7:  return "WDT";
        case 8:  return "DEEPSLEEP";
        case 9:  return "BROWNOUT";
        case 10: return "SDIO";
        default: return "UNKNOWN";
    }
}

static const char *alert_category_str(alert_category_t c)
{
    switch (c) {
        case ALERT_CATEGORY_PROCESS:   return "PROCESS";
        case ALERT_CATEGORY_SYSTEM:    return "SYSTEM";
        case ALERT_CATEGORY_SECURITY:  return "SECURITY";
        default: return "UNKNOWN";
    }
}

static const char *alert_severity_str(alert_severity_t s)
{
    switch (s) {
        case ALERT_SEVERITY_CRITICAL: return "CRITICAL";
        case ALERT_SEVERITY_HIGH:     return "HIGH";
        case ALERT_SEVERITY_WARNING:  return "WARNING";
        case ALERT_SEVERITY_INFO:     return "INFO";
        default: return "UNKNOWN";
    }
}

static void format_iso8601(uint64_t epoch_s, char *out, size_t out_len)
{
    if (!out || out_len == 0) return;
    if (epoch_s == 0 || !g_gs.time_valid) {
        snprintf(out, out_len, "1970-01-01T00:00:00Z");
        return;
    }
    time_t t = (time_t)epoch_s;
    struct tm tm_info;
    if (localtime_r(&t, &tm_info) == NULL) {
        snprintf(out, out_len, "1970-01-01T00:00:00Z");
        return;
    }
    snprintf(out, out_len, "%04d-%02d-%02dT%02d:%02d:%02d",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
}

/* @requirement RF-WEB-002 / RF-DADOS-001 Schema canônico completo do GlobalState. */
static esp_err_t state_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    const char *st = "UNKNOWN";
    switch (g_gs.system_state) {
        case SYSTEM_STATE_NORMAL:    st = "NORMAL";    break;
        case SYSTEM_STATE_DEGRADED:  st = "DEGRADED";  break;
        case SYSTEM_STATE_SAFE_OFF:  st = "SAFE_OFF";  break;
        case SYSTEM_STATE_EMERGENCY: st = "EMERGENCY"; break;
        default: break;
    }
    const char *reason = "NONE";
    switch (g_gs.safeoff_reason) {
        case SAFEOFF_REASON_NONE:             reason = "NONE";           break;
        case SAFEOFF_REASON_THERMAL_CRITICAL: reason = "THERMAL";        break;
        case SAFEOFF_REASON_ATO_OVERFLOW:     reason = "ATO_OVERFLOW";   break;
        case SAFEOFF_REASON_ELECTRIC_TOTAL:   reason = "ELECTRIC";       break;
        case SAFEOFF_REASON_SELFTEST_FAIL:    reason = "SELFTEST_FAIL";  break;
        case SAFEOFF_REASON_MANUAL_CRITICAL:  reason = "MANUAL";         break;
        default:                              reason = "OTHER";          break;
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "system_state", st);
    cJSON_AddStringToObject(json, "safeoff_reason", reason);
    cJSON_AddStringToObject(json, "safeoff_source_alm", g_gs.safeoff_source_alm);
    cJSON_AddStringToObject(json, "safeoff_entered_at", g_gs.safeoff_entered_at);
    cJSON_AddBoolToObject(json, "restart_in_progress", g_gs.restart_in_progress);
    cJSON_AddBoolToObject(json, "wizard_completed", g_gs.wizard_completed);
    cJSON_AddNumberToObject(json, "wizard_step", (double)g_gs.wizard_step);
    cJSON_AddBoolToObject(json, "monitor_only_mode", g_gs.monitor_only_mode);
    cJSON_AddBoolToObject(json, "maintenance_mode", g_gs.maintenance_mode);
    cJSON_AddBoolToObject(json, "observation_mode", g_gs.observation_mode);
    cJSON_AddBoolToObject(json, "selftest_passed", g_gs.selftest_passed);
    cJSON_AddBoolToObject(json, "temp_ok", g_gs.temp_ok);
    cJSON_AddBoolToObject(json, "ato_ok", g_gs.ato_ok);
    cJSON_AddBoolToObject(json, "pzem_ok", g_gs.pzem_ok);
    cJSON_AddBoolToObject(json, "electric_ok", g_gs.electric_ok);
    cJSON_AddBoolToObject(json, "sd_ok", g_gs.sd_ok);
    cJSON_AddBoolToObject(json, "wifi_ok", g_gs.wifi_ok);
    cJSON_AddBoolToObject(json, "ui_ok", g_gs.ui_ok);
    cJSON_AddBoolToObject(json, "hw_ok", g_gs.hw_ok);
    cJSON_AddNumberToObject(json, "active_alm_categories", (double)g_gs.active_alm_categories);
    cJSON_AddNumberToObject(json, "alerts_active", (double)alert_manager_active_count());
    cJSON_AddNumberToObject(json, "critical_alerts_count", (double)g_gs.critical_alerts_count);
    cJSON_AddNumberToObject(json, "last_reset_reason", (double)g_gs.last_reset_reason);
    cJSON_AddNumberToObject(json, "uptime_s", (double)g_gs.uptime_s);
    cJSON_AddNumberToObject(json, "temp_filtered_c", (double)g_gs.temp_filtered_c);
    cJSON_AddBoolToObject(json, "feed_active", g_gs.feed_active);
    cJSON_AddNumberToObject(json, "feed_remaining_s", (double)g_gs.feed_remaining_s);
    cJSON_AddBoolToObject(json, "time_valid", g_gs.time_valid);
    cJSON_AddStringToObject(json, "time_source", time_source_str(g_gs.time_source));
    cJSON_AddStringToObject(json, "config_schema_version", g_gs.config_schema_version);
    cJSON_AddStringToObject(json, "firmware_version", g_gs.fw_version);
    cJSON_AddStringToObject(json, "srs_version", g_gs.srs_version);
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t health_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "state", system_state_to_str(g_gs.system_state));
    cJSON_AddNumberToObject(json, "system_state", g_gs.system_state);
    cJSON_AddBoolToObject(json, "temp_ok", g_gs.temp_ok);
    cJSON_AddBoolToObject(json, "ato_ok", g_gs.ato_ok);
    cJSON_AddBoolToObject(json, "pzem_ok", g_gs.pzem_ok);
    cJSON_AddBoolToObject(json, "electric_ok", g_gs.electric_ok);
    cJSON_AddBoolToObject(json, "maintenance_mode", g_gs.maintenance_mode);
    cJSON_AddBoolToObject(json, "selftest_passed", g_gs.selftest_passed);
    cJSON_AddNumberToObject(json, "uptime_s", (double)g_gs.uptime_s);
    cJSON_AddNumberToObject(json, "alerts_count", g_gs.active_alerts_count);
    cJSON_AddStringToObject(json, "version", g_gs.fw_version);
    cJSON_AddStringToObject(json, "last_reset_reason", reset_reason_str(g_gs.last_reset_reason));
    cJSON_AddNumberToObject(json, "last_reset_reason_code", (double)g_gs.last_reset_reason);
    cJSON_AddNumberToObject(json, "heap_free_kb", (double)(esp_get_free_heap_size() / 1024));
    cJSON_AddNumberToObject(json, "wdt_resets_24h", (double)wdt_stats_get_resets_24h());
    {
        char ts[32];
        format_iso8601(g_gs.last_health_check_timestamp, ts, sizeof(ts));
        cJSON_AddStringToObject(json, "last_health_check_timestamp", ts);
    }

    /* @requirement RF-WEB-005 Campos adicionais de saúde. */
    cJSON_AddBoolToObject(json, "monitor_only_mode", g_gs.monitor_only_mode);
    cJSON_AddBoolToObject(json, "time_valid", g_gs.time_valid);
    cJSON_AddStringToObject(json, "time_source", time_source_str(g_gs.time_source));
    cJSON_AddBoolToObject(json, "sd_mounted", storage_sd_is_mounted());
    {
        int32_t sd_free_mb = -1;
        if (storage_sd_is_mounted()) {
            uint64_t total = 0;
            uint64_t free = 0;
            if (storage_sd_get_space(&total, &free) == ESP_OK) {
                sd_free_mb = (int32_t)(free / (1024ULL * 1024ULL));
            }
        }
        cJSON_AddNumberToObject(json, "sd_free_mb", (double)sd_free_mb);
    }
    cJSON_AddNumberToObject(json, "wifi_rssi_dbm", (double)wifi_ctl_sta_get_rssi());

    /* @requirement RNF-RESILIENCE-001 Estados dos circuit breakers por barramento. */
    static const char *cb_names[CB_COUNT] = {
        "I2C", "SPI_ADC", "SPI_DISPLAY", "SPI_SD", "UART_PZEM", "DS18B20"
    };
    static const char *cb_state_names[] = { "CLOSED", "OPEN", "HALF_OPEN" };
    cJSON *cb = cJSON_CreateObject();
    for (int i = 0; i < CB_COUNT; i++) {
        cb_state_t s = circuit_breaker_get_state((cb_bus_id_t)i);
        cJSON_AddStringToObject(cb, cb_names[i],
            (s <= CB_STATE_HALF_OPEN) ? cb_state_names[s] : "UNKNOWN");
    }
    cJSON_AddItemToObject(json, "circuit_breaker_states", cb);

    /* @requirement RF-FLOW-SELFTEST-002/003 Resultados por subsistema (PASS/FAIL/
     * NOT_RUN/SKIPPED) observáveis via API. */
    cJSON *stj = cJSON_CreateObject();
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        const selftest_result_t *r = self_test_get_result((selftest_id_t)i);
        if (r) {
            cJSON_AddStringToObject(stj, r->name, self_test_status_str(r->status));
        }
    }
    cJSON_AddItemToObject(json, "selftest", stj);

    /* @requirement RF-HEALTH-MATRIX-001 / RF-WEB-005 Expõe a matriz de saúde. */
    static const char *sub_names[SUB_COUNT] = {
        "TEMP", "PH", "LEVEL", "FLOW", "CURRENT", "VOLTAGE",
        "BUS_SPI", "BUS_I2C", "BUS_1WIRE", "NVS", "SD", "WIFI", "RTC", "RELAY_BOARD"
    };
    static const char *health_status_names[] = {
        "OK", "DEGRADED", "FAILED", "OPEN", "HALF_OPEN", "CLOSED", "UNKNOWN"
    };
    cJSON_AddStringToObject(json, "health_aggregate", health_status_names[health_aggregate()]);
    cJSON *matrix = cJSON_CreateObject();
    for (int i = 0; i < SUB_COUNT; i++) {
        const health_entry_t *e = health_get_entry((subsystem_id_t)i);
        cJSON_AddStringToObject(matrix, sub_names[i],
            e ? health_status_names[e->status] : "UNKNOWN");
    }
    cJSON_AddItemToObject(json, "subsystems", matrix);
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t plugs_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < 10; i++) {
        plug_model_t *pm = plug_manager_get((plug_id_t)(i + 1));
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "id", i + 1);
        cJSON_AddStringToObject(p, "name", pm ? pm->name : "");
        cJSON_AddNumberToObject(p, "type", pm ? (int)pm->type : 0);
        cJSON_AddNumberToObject(p, "mode", pm ? (int)pm->mode : 0);
        cJSON_AddBoolToObject(p, "state", relay_get((uint8_t)(i + 1)));
        cJSON_AddBoolToObject(p, "command_allowed", g_gs.system_state == SYSTEM_STATE_NORMAL || g_gs.system_state == SYSTEM_STATE_DEGRADED);
        cJSON_AddBoolToObject(p, "blocked_by_safe_state", pm ? pm->blocked_by_safe_state : false);
        cJSON_AddBoolToObject(p, "is_critical", pm ? pm->is_critical : false);
        float cur = 0;
        acs712_read_plug((uint8_t)(i + 1), &cur);
        cJSON_AddNumberToObject(p, "current_a", (double)cur);
        cJSON_AddNumberToObject(p, "power_w", pm ? (double)pm->power_w : 0);
        cJSON_AddNumberToObject(p, "energy_wh_today", pm ? (double)pm->energy_wh_today : 0);
        cJSON_AddItemToArray(arr, p);
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "plugs", arr);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t plug_set_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *id_item = cJSON_GetObjectItem(json, "id");
    cJSON *state_item = cJSON_GetObjectItem(json, "state");
    cJSON *confirm_item = cJSON_GetObjectItem(json, "confirm");
    if (!cJSON_IsNumber(id_item) || !cJSON_IsBool(state_item)) {
        cJSON_Delete(json);
        return send_error(req, "missing_fields", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    int plug_id = id_item->valueint;
    bool target_state = cJSON_IsTrue(state_item);
    bool confirmed = cJSON_IsTrue(confirm_item);
    cJSON_Delete(json);

    if (plug_id < 1 || plug_id > 10) {
        return send_error(req, "invalid_plug_id", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    cmd_validation_t cv = command_validator_can_toggle_plug(&g_gs, (uint8_t)plug_id, target_state);
    if (!cv.allowed) {
        return send_error(req, cv.error_code ? cv.error_code : "forbidden",
                          ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    /* @requirement RF-PLUG-011 Dupla confirmação obrigatória para relé crítico. */
    if (cv.requires_double_confirmation && !confirmed) {
        return send_error(req, "double_confirmation_required",
                          ERR_VALIDATION_ERROR, 409, "409 Conflict");
    }

    esp_err_t err = plug_manager_toggle((plug_id_t)plug_id, target_state);
    if (err != ESP_OK) {
        return send_error(req, "plug_denied", ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    /* ALM-065 + DEGRADED em desligamento crítico: tratado em plug_manager_toggle_ex. */

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "id", plug_id);
    cJSON_AddBoolToObject(resp, "state", target_state);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t plug_mode_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *id_item = cJSON_GetObjectItem(json, "id");
    cJSON *mode_item = cJSON_GetObjectItem(json, "mode");
    if (!cJSON_IsNumber(id_item) || !cJSON_IsNumber(mode_item)) {
        cJSON_Delete(json);
        return send_error(req, "missing_fields", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    int plug_id = id_item->valueint;
    int mode = mode_item->valueint;
    cJSON_Delete(json);

    if (plug_id < 1 || plug_id > 10) {
        return send_error(req, "invalid_plug_id", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    cmd_validation_t cv = command_validator_can_set_mode(&g_gs, (uint8_t)plug_id);
    if (!cv.allowed) {
        return send_error(req, cv.error_code, ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    esp_err_t err = plug_manager_set_mode((plug_id_t)plug_id, (plug_mode_t)mode);
    if (err != ESP_OK) {
        return send_error(req, "mode_failed", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "id", plug_id);
    cJSON_AddNumberToObject(resp, "mode", mode);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t sensors_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
    float temp = 25.0f;
    ds18b20_read(&temp);
    cJSON_AddNumberToObject(json, "temp_c", (double)temp);
    cJSON_AddBoolToObject(json, "temp_valid", true);
    uint16_t ato_adc = 0;
    mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &ato_adc);
    cJSON_AddNumberToObject(json, "ato_level_adc", ato_adc);
    uint16_t raw = 0;
    mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &raw);
    cJSON_AddNumberToObject(json, "keypad_adc", raw);
    ds3231_time_t rt = {0};
    if (ds3231_get_time(&rt) == ESP_OK) {
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "%04d-%02d-%02dT%02d:%02d:%02d",
            rt.year, rt.month, rt.day, rt.hour, rt.minute, rt.second);
        cJSON_AddStringToObject(json, "rtc_time", time_str);
    }
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t energy_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "voltage_v", (double)g_pzem.voltage_v);
    cJSON_AddNumberToObject(json, "current_a", (double)g_pzem.current_a);
    cJSON_AddNumberToObject(json, "power_w", (double)g_pzem.power_w);
    cJSON_AddNumberToObject(json, "energy_wh", (double)g_pzem.energy_wh);
    cJSON_AddNumberToObject(json, "frequency_hz", (double)g_pzem.frequency_hz);
    cJSON_AddNumberToObject(json, "pf", (double)g_pzem.pf);
    cJSON_AddBoolToObject(json, "valid", g_pzem.valid);
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t alerts_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    uint64_t now_s = esp_timer_get_time() / USEC_PER_SEC;
    cJSON *arr = cJSON_CreateArray();
    for (int16_t id = 1; id <= 65; id++) {
        const alert_slot_t *aslot = alert_manager_get_slot(id);
        if (!aslot || !aslot->active) continue;
        cJSON *a = cJSON_CreateObject();
        cJSON_AddNumberToObject(a, "id", id);
        cJSON_AddStringToObject(a, "severity", alert_severity_str(aslot->severity));
        cJSON_AddStringToObject(a, "category", alert_category_str(aslot->category));
        cJSON_AddStringToObject(a, "message", aslot->message);
        {
            char ts[32];
            format_iso8601(aslot->last_seen_ts, ts, sizeof(ts));
            cJSON_AddStringToObject(a, "timestamp", ts);
        }
        cJSON_AddBoolToObject(a, "acked", aslot->acked);
        cJSON_AddBoolToObject(a, "silenced", alert_manager_is_silenced(id, now_s));
        cJSON_AddStringToObject(a, "action_hint", aslot->action_hint);
        cJSON_AddItemToArray(arr, a);
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "alerts", arr);
    cJSON_AddNumberToObject(resp, "count", g_gs.active_alerts_count);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t alert_ack_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    uint16_t specific_id = 0;
    if (body) {
        cJSON *json = cJSON_Parse(body);
        if (json) {
            cJSON *id_item = cJSON_GetObjectItem(json, "alert_id");
            if (cJSON_IsNumber(id_item)) {
                specific_id = (uint16_t)id_item->valueint;
            }
            cJSON_Delete(json);
        }
        free(body);
    }

    cmd_validation_t cv = command_validator_can_ack_alert(&g_gs, specific_id);
    if (!cv.allowed) {
        return send_error(req, cv.error_code, ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    uint64_t now = esp_timer_get_time() / USEC_PER_SEC;
    int acked = 0;
    if (specific_id > 0) {
        if (alert_manager_is_active(specific_id)) {
            if (alert_manager_ack(specific_id, now)) {
                acked++;
                safe_state_ack_on_alert_ack((int16_t)specific_id, now);
            }
        }
    } else {
        for (int16_t id = 1; id <= 65; id++) {
            if (alert_manager_is_active(id)) {
                if (alert_manager_ack(id, now)) {
                    acked++;
                    safe_state_ack_on_alert_ack(id, now);
                }
            }
        }
    }
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "acked", acked);
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t command_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *cmd_item = cJSON_GetObjectItem(json, "cmd");
    if (!cJSON_IsString(cmd_item)) {
        cJSON_Delete(json);
        return send_error(req, "missing_cmd", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    const char *cmd = cmd_item->valuestring;

    cmd_validation_t res = { .allowed = false, .requires_double_confirmation = false, .error_code = "unknown_cmd" };
    const char *status_msg = "command_accepted";

    if (strcmp(cmd, "toggle_plug") == 0) {
        cJSON *plug_item = cJSON_GetObjectItem(json, "plug_id");
        cJSON *state_item = cJSON_GetObjectItem(json, "state");
        cJSON *confirm_item = cJSON_GetObjectItem(json, "confirm");
        bool confirmed = cJSON_IsTrue(confirm_item);
        if (cJSON_IsNumber(plug_item) && cJSON_IsBool(state_item)) {
            uint8_t plug_id = (uint8_t)plug_item->valueint;
            bool desired_on = cJSON_IsTrue(state_item);
            res = command_validator_can_toggle_plug(&g_gs, plug_id, desired_on);
            if (res.allowed) {
                /* @requirement RF-PLUG-011 Dupla confirmação para relé crítico. */
                if (res.requires_double_confirmation && !confirmed) {
                    res.allowed = false;
                    res.error_code = "double_confirmation_required";
                } else {
                    /* @requirement RF-PLUG-001 Rota única: atuação via plug_manager. */
                    esp_err_t act = plug_manager_toggle((plug_id_t)plug_id, desired_on);
                    if (act != ESP_OK) {
                        res.allowed = false;
                        res.error_code = "actuation_blocked_by_safety";
                    } else {
                        status_msg = "plug_toggled";
                    }
                }
            }
        } else {
            res.error_code = "missing_plug_params";
        }
    } else if (strcmp(cmd, "start_feed") == 0) {
        res = command_validator_can_start_feed(&g_gs);
        if (res.allowed) {
            g_feed_request = true;
            status_msg = "feed_started";
        }
    } else if (strcmp(cmd, "restart") == 0) {
        res = command_validator_can_restart(&g_gs);
        if (res.allowed) {
            g_gs.restart_in_progress = true;
            audit_log_event(AUDIT_SAFE_OFF, "Restart requested via API");
            status_msg = "restart_initiated";
        }
    } else if (strcmp(cmd, "ack_all") == 0) {
        /* @requirement RF-WEB-006 ACK em massa também passa pelo validador. */
        res = command_validator_can_ack_alert(&g_gs, 0);
        if (res.allowed) {
            uint64_t now = esp_timer_get_time() / USEC_PER_SEC;
            int acked = 0;
            for (int16_t id = 1; id <= 65; id++) {
                if (alert_manager_is_active(id)) {
                    if (alert_manager_ack(id, now)) acked++;
                }
            }
            status_msg = "alerts_acked";
            cJSON_AddNumberToObject(json, "acked", acked);
        }
    }

    cJSON_Delete(json);
    if (!res.allowed) {
        return send_error(req, res.error_code ? res.error_code : "forbidden", ERR_VALIDATION_ERROR, 403, "403 Forbidden");
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", status_msg);
    cJSON_AddStringToObject(resp, "cmd", cmd);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "wizard_completed", g_gs.wizard_completed);
    cJSON_AddBoolToObject(json, "monitor_only_mode", g_gs.monitor_only_mode);
    cJSON_AddNumberToObject(json, "safeoff_reason", g_gs.safeoff_reason);
    cJSON_AddStringToObject(json, "safeoff_source_alm", g_gs.safeoff_source_alm);
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t config_set_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *mo = cJSON_GetObjectItem(json, "monitor_only_mode");
    if (cJSON_IsBool(mo)) {
        cmd_validation_t cv = command_validator_can_set_config(&g_gs, "monitor_only_mode");
        if (!cv.allowed) {
            cJSON_Delete(json);
            return send_error(req, cv.error_code, ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
        }
        bool v = cJSON_IsTrue(mo);
        /* @requirement RF-STORAGE-001 Persistir em NVS (não só em RAM). */
        config_set_monitor_only(v);
        g_gs.monitor_only_mode = v;
        audit_log_event(AUDIT_CONFIG_CHANGE,
                        v ? "monitor_only_mode=on via API" : "monitor_only_mode=off via API");
    }
    cJSON *wiz = cJSON_GetObjectItem(json, "wizard_completed");
    if (cJSON_IsBool(wiz)) {
        cmd_validation_t cvw = command_validator_can_set_config(&g_gs, "wizard_completed");
        if (!cvw.allowed) {
            cJSON_Delete(json);
            return send_error(req, cvw.error_code ? cvw.error_code : "forbidden",
                              ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
        }
        bool v = cJSON_IsTrue(wiz);
        config_set_wizard_completed(v);
        g_gs.wizard_completed = v;
        audit_log_event(AUDIT_CONFIG_CHANGE, "wizard_completed updated via API");
    }
    cJSON_Delete(json);
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "config_updated");
    return send_json_resp(req, resp, "200 OK");
}

static bool parse_audit_ts(const char *line, double *ts_out, const char **msg_out)
{
    if (!line || line[0] != '[') return false;
    const char *end = strchr(line + 1, ']');
    if (!end) return false;
    char ts_buf[32];
    size_t len = (size_t)(end - (line + 1));
    if (len >= sizeof(ts_buf)) return false;
    memcpy(ts_buf, line + 1, len);
    ts_buf[len] = '\0';
    *ts_out = atof(ts_buf);
    const char *msg = end + 1;
    while (*msg == ' ') msg++;
    if (msg_out) *msg_out = msg;
    return true;
}

static void tail_log_file(const char *path, cJSON *arr, const char *source, int max_lines)
{
    FILE *f = fopen(path, "r");
    if (!f) return;

    char buf[256];
    char tail[10][256];
    int tcount = 0;
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }
        if (len == 0) continue;
        if (tcount < max_lines) {
            strncpy(tail[tcount], buf, 255);
            tail[tcount][255] = '\0';
            tcount++;
        } else {
            memmove(tail[0], tail[1], (size_t)(max_lines - 1) * sizeof(tail[0]));
            strncpy(tail[max_lines - 1], buf, 255);
            tail[max_lines - 1][255] = '\0';
        }
    }
    fclose(f);

    for (int i = 0; i < tcount; i++) {
        cJSON *e = cJSON_CreateObject();
        double ts = 0;
        const char *msg = tail[i];
        if (parse_audit_ts(tail[i], &ts, &msg)) {
            cJSON_AddNumberToObject(e, "ts", ts);
            cJSON_AddStringToObject(e, "msg", msg);
        } else {
            cJSON_AddStringToObject(e, "msg", tail[i]);
        }
        cJSON_AddStringToObject(e, "level", "audit");
        if (source) cJSON_AddStringToObject(e, "source", source);
        cJSON_AddItemToArray(arr, e);
    }
}

static esp_err_t log_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *arr = cJSON_CreateArray();

    char ram_lines[32][256];
    uint16_t n = storage_facade_audit_read_recent(ram_lines, 32);
    for (uint16_t i = 0; i < n; i++) {
        cJSON *e = cJSON_CreateObject();
        double ts = 0;
        const char *msg = ram_lines[i];
        if (parse_audit_ts(ram_lines[i], &ts, &msg)) {
            cJSON_AddNumberToObject(e, "ts", ts);
            cJSON_AddStringToObject(e, "msg", msg);
        } else {
            cJSON_AddStringToObject(e, "msg", ram_lines[i]);
        }
        cJSON_AddStringToObject(e, "level", "audit");
        cJSON_AddStringToObject(e, "source", "ram");
        cJSON_AddItemToArray(arr, e);
    }

    if (storage_sd_is_mounted()) {
        tail_log_file("/sdcard/logs/security/log.txt", arr, "sd_security", 10);
        tail_log_file("/sdcard/logs/events/log.txt", arr, "sd_events", 10);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "entries", arr);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t config_export_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = NULL;
    if (config_export_to_json(&json) != ESP_OK || !json) {
        return send_error(req, "export_failed", ERR_INTERNAL, 500, "500 Internal Server Error");
    }
    esp_err_t err = send_json_resp(req, json, "200 OK");
    cJSON_Delete(json);
    return err;
}

static esp_err_t config_import_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cmd_validation_t cv = command_validator_can_set_config(&g_gs, "import");
    if (!cv.allowed) {
        cJSON_Delete(json);
        return send_error(req, cv.error_code ? cv.error_code : "forbidden",
                          ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    bool dry_run = false;
    cJSON *dr = cJSON_GetObjectItem(json, "dry_run");
    if (cJSON_IsTrue(dr)) dry_run = true;

    bool valid = false;
    uint32_t crc = 0;
    esp_err_t imp = config_import_from_json(json, dry_run, &valid, &crc);
    cJSON_Delete(json);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "valid", valid);
    cJSON_AddNumberToObject(resp, "crc32", (double)crc);
    cJSON_AddBoolToObject(resp, "dry_run", dry_run);
    if (imp != ESP_OK) {
        cJSON_AddStringToObject(resp, "status", "rejected");
        send_json_resp(req, resp, "422 Unprocessable Entity");
        cJSON_Delete(resp);
        return ESP_OK;
    }
    cJSON_AddStringToObject(resp, "status", dry_run ? "preview_ok" : "imported");
    if (!dry_run) {
        audit_log_event(AUDIT_CONFIG_CHANGE, "config imported via API");
    }
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t login_handler(httpd_req_t *req)
{
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *user = cJSON_GetObjectItem(json, "user");
    cJSON *pass = cJSON_GetObjectItem(json, "password");
    if (!cJSON_IsString(user) || !cJSON_IsString(pass)) {
        cJSON_Delete(json);
        return send_error(req, "missing_fields", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    /* @requirement RNF-SECURITY-001 Rate limit por IP também na rota de login. */
    uint32_t ip = client_ip_of(req);
    if (!rate_limit_check(ip)) {
        cJSON_Delete(json);
        audit_log_event(AUDIT_LOGIN, "login rate-limited");
        return send_error(req, "rate_limited", ERR_RATE_LIMITED, 429, "429 Too Many Requests");
    }
    const char *token = api_auth_login(user->valuestring, pass->valuestring, ip);
    cJSON_Delete(json);
    if (!token) {
        /* @requirement RNF-SECURITY-003 Auditar falha de login. */
        audit_log_event(AUDIT_LOGIN, "login failed (invalid credentials)");
        return send_error(req, "invalid_credentials", ERR_AUTH_REQUIRED, 401, "401 Unauthorized");
    }
    /* @requirement RNF-SECURITY-001 Sucesso zera o contador de rate limit do IP. */
    rate_limit_reset(ip);
    audit_log_event(AUDIT_LOGIN, "login success (admin)");
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "token", token);
    cJSON_AddNumberToObject(resp, "expires_in_s", (60 * 60));
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t logout_handler(httpd_req_t *req)
{
    size_t buf_len = httpd_req_get_hdr_value_len(req, "X-Auth-Token") + 1;
    if (buf_len > 1) {
        char *token = malloc(buf_len);
        if (token && httpd_req_get_hdr_value_str(req, "X-Auth-Token", token, buf_len) == ESP_OK) {
            api_auth_logout(token);
        }
        free(token);
    }
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "status", "logged_out");
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t calibrate_handler(httpd_req_t *req)
{
    AUTH_GUARD();

    if (req->method == HTTP_GET) {
        const calibration_params_storage_t *cal = config_get_calibration();
        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "ato_zero_offset_adc", cal->ato_zero_offset_adc);
        cJSON_AddNumberToObject(json, "temp_offset_c", (double)cal->temp_offset_c);
        cJSON *arr = cJSON_CreateArray();
        for (int i = 0; i < 10; i++) {
            cJSON_AddItemToArray(arr, cJSON_CreateNumber((double)cal->acs712_zero_offset_mv[i]));
        }
        cJSON_AddItemToObject(json, "acs712_zero_offsets_mv", arr);
        return send_json_resp(req, json, "200 OK");
    }

    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cmd_validation_t cv = command_validator_can_calibrate(&g_gs);
    if (!cv.allowed) {
        cJSON_Delete(json);
        return send_error(req, cv.error_code, ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    cJSON *sensor = cJSON_GetObjectItem(json, "sensor");
    if (!cJSON_IsString(sensor)) {
        cJSON_Delete(json);
        return send_error(req, "missing_sensor", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    const char *sensor_name = sensor->valuestring;
    calibration_params_storage_t cal = *config_get_calibration();
    bool updated = false;

    if (strcmp(sensor_name, "ato_zero") == 0) {
        uint16_t adc = 0;
        esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &adc);
        if (err != ESP_OK) {
            cJSON_Delete(json);
            return send_error(req, "read_failed", ERR_INTERNAL, 500, "500 Internal Server Error");
        }
        cal.ato_zero_offset_adc = (int32_t)adc;
        updated = true;
    } else if (strcmp(sensor_name, "plug_zero") == 0) {
        cJSON *plug = cJSON_GetObjectItem(json, "plug_id");
        int plug_id = cJSON_IsNumber(plug) ? plug->valueint : 0;
        if (plug_id < 1 || plug_id > 10) {
            cJSON_Delete(json);
            return send_error(req, "invalid_plug_id", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
        }
        esp_err_t err = acs712_calibrate_zero((uint8_t)plug_id);
        if (err != ESP_OK) {
            cJSON_Delete(json);
            return send_error(req, "calibrate_failed", ERR_INTERNAL, 500, "500 Internal Server Error");
        }
        cal.acs712_zero_offset_mv[plug_id - 1] = acs712_get_zero_offset((uint8_t)plug_id);
        updated = true;
    } else {
        cJSON_Delete(json);
        return send_error(req, "unknown_sensor", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    if (updated) {
        config_set_calibration(&cal);
    }

    cJSON_Delete(json);
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "calibrated");
    cJSON_AddStringToObject(resp, "sensor", sensor_name);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t feed_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cmd_validation_t cv = command_validator_can_start_feed(&g_gs);
    if (!cv.allowed) {
        return send_error(req, cv.error_code, ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    /* @requirement RF-FEED-002 Ativação do Feed exige confirmação explícita ("confirm":true). */
    bool confirmed = false;
    char *body = read_body(req);
    if (body) {
        cJSON *json = cJSON_Parse(body);
        free(body);
        if (json) {
            confirmed = cJSON_IsTrue(cJSON_GetObjectItem(json, "confirm"));
            cJSON_Delete(json);
        }
    }
    if (cv.requires_double_confirmation && !confirmed) {
        return send_error(req, "confirmation_required", ERR_VALIDATION_ERROR, 409, "409 Conflict");
    }

    g_feed_request = true;
    audit_log_event(AUDIT_FEED_MODE, "Feed mode requested via API");
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "feed_started");
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t system_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "heap_total_bytes", esp_get_free_heap_size() + esp_get_free_internal_heap_size());
    cJSON_AddNumberToObject(json, "heap_free_bytes", esp_get_free_heap_size());
    cJSON_AddNumberToObject(json, "heap_free_internal", esp_get_free_internal_heap_size());
    cJSON_AddStringToObject(json, "fw_version", g_gs.fw_version);
    cJSON_AddStringToObject(json, "srs_version", g_gs.srs_version);
    cJSON_AddNumberToObject(json, "uptime_s", (double)g_gs.uptime_s);
    cJSON_AddNumberToObject(json, "last_reset_reason", g_gs.last_reset_reason);
    cJSON_AddBoolToObject(json, "time_valid", g_gs.time_valid);
    cJSON_AddNumberToObject(json, "time_source", g_gs.time_source);
    cJSON_AddBoolToObject(json, "time_valid", g_gs.time_valid);
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t auth_password_handler(httpd_req_t *req)
{
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *pass = cJSON_GetObjectItem(json, "password");
    if (!cJSON_IsString(pass) || strlen(pass->valuestring) < 4) {
        cJSON_Delete(json);
        return send_error(req, "invalid_password", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    esp_err_t err = api_auth_set_password(pass->valuestring);
    cJSON_Delete(json);
    if (err != ESP_OK) {
        return send_error(req, "password_set_failed", ERR_INTERNAL, 500, "500 Internal Server Error");
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "password_set");
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t wizard_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        cJSON *json = cJSON_CreateObject();
        cJSON_AddBoolToObject(json, "wizard_completed", g_gs.wizard_completed);
        cJSON_AddBoolToObject(json, "password_set", api_auth_has_password());
        cJSON_AddNumberToObject(json, "wizard_step", g_gs.wizard_step);
        return send_json_resp(req, json, "200 OK");
    }

    /* @requirement RF-WEB-006 O wizard só é aberto no setup inicial. Após concluído,
     * qualquer escrita de configuração via /wizard exige autenticação. */
    if (g_gs.wizard_completed) {
        AUTH_GUARD();
    }

    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *step = cJSON_GetObjectItem(json, "wizard_step");
    if (cJSON_IsNumber(step)) {
        int new_step = (int)(step->valueint);
        if (new_step >= 0 && new_step <= 6) {
            g_gs.wizard_step = (uint8_t)new_step;
            config_set_wizard_step((uint8_t)new_step);
        }
    }

    cJSON *wiz = cJSON_GetObjectItem(json, "wizard_completed");
    if (cJSON_IsBool(wiz) && cJSON_IsTrue(wiz)) {
        g_gs.wizard_completed = true;
        g_gs.wizard_step = 6;
        config_set_wizard_completed(true);
        config_set_wizard_step(6);
    }

    cJSON *t = cJSON_GetObjectItem(json, "thermal");
    if (cJSON_IsObject(t)) {
        thermal_params_storage_t tp = *config_get_thermal();
        cJSON *v;
        v = cJSON_GetObjectItem(t, "temp_normal_c"); if (cJSON_IsNumber(v)) tp.temp_normal_c = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(t, "temp_critical_c"); if (cJSON_IsNumber(v)) tp.temp_critical_c = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(t, "hysteresis_c"); if (cJSON_IsNumber(v)) tp.hysteresis_c = (float)(v->valuedouble);
        /* @requirement RF-WEB-003 Validação de consistência (sem inventar limites):
         * normal < crítico; histerese não-negativa e abaixo da banda normal→crítico. */
        if (!(tp.temp_normal_c < tp.temp_critical_c) ||
            tp.hysteresis_c < 0.0f ||
            tp.hysteresis_c >= (tp.temp_critical_c - tp.temp_normal_c)) {
            cJSON_Delete(json);
            return send_error(req, "invalid_thermal_params", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
        }
        config_set_thermal(&tp);
        audit_log_event(AUDIT_CONFIG_CHANGE, "wizard: thermal params updated");
    }

    cJSON *a = cJSON_GetObjectItem(json, "ato");
    if (cJSON_IsObject(a)) {
        ato_params_storage_t ap = *config_get_ato();
        cJSON *v;
        v = cJSON_GetObjectItem(a, "low_level_adc"); if (cJSON_IsNumber(v)) ap.low_level_adc = v->valueint;
        v = cJSON_GetObjectItem(a, "high_level_adc"); if (cJSON_IsNumber(v)) ap.high_level_adc = v->valueint;
        v = cJSON_GetObjectItem(a, "overflow_margin_adc"); if (cJSON_IsNumber(v)) ap.overflow_margin_adc = v->valueint;
        v = cJSON_GetObjectItem(a, "refill_timeout_s"); if (cJSON_IsNumber(v)) ap.refill_timeout_s = (uint32_t)(v->valueint);
        /* @requirement RF-WEB-003 ADC 12 bits (0..4095); low < high; timeout > 0. */
        if (ap.low_level_adc < 0 || ap.high_level_adc > 4095 ||
            !(ap.low_level_adc < ap.high_level_adc) ||
            ap.overflow_margin_adc < 0 || ap.refill_timeout_s == 0) {
            cJSON_Delete(json);
            return send_error(req, "invalid_ato_params", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
        }
        config_set_ato(&ap);
        audit_log_event(AUDIT_CONFIG_CHANGE, "wizard: ato params updated");
    }

    cJSON *e = cJSON_GetObjectItem(json, "electric");
    if (cJSON_IsObject(e)) {
        electric_params_storage_t ep = *config_get_electric();
        cJSON *v;
        v = cJSON_GetObjectItem(e, "total_power_limit_w"); if (cJSON_IsNumber(v)) ep.total_power_limit_w = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(e, "overvoltage_limit_v"); if (cJSON_IsNumber(v)) ep.overvoltage_limit_v = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(e, "undervoltage_limit_v"); if (cJSON_IsNumber(v)) ep.undervoltage_limit_v = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(e, "per_plug_current_limit_a"); if (cJSON_IsNumber(v)) ep.per_plug_current_limit_a = (float)(v->valuedouble);
        /* @requirement RF-WEB-003 Limites positivos; subtensão < sobretensão. */
        if (ep.total_power_limit_w <= 0.0f || ep.per_plug_current_limit_a <= 0.0f ||
            !(ep.undervoltage_limit_v < ep.overvoltage_limit_v)) {
            cJSON_Delete(json);
            return send_error(req, "invalid_electric_params", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
        }
        config_set_electric(&ep);
        audit_log_event(AUDIT_CONFIG_CHANGE, "wizard: electric params updated");
    }

    if (cJSON_IsBool(wiz) && cJSON_IsTrue(wiz)) {
        audit_log_event(AUDIT_CONFIG_CHANGE, "wizard completed");
    }

    cJSON_Delete(json);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "wizard_completed", g_gs.wizard_completed);
    cJSON_AddNumberToObject(resp, "wizard_step", g_gs.wizard_step);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t reset_api_handler(httpd_req_t *req)
{
    AUTH_GUARD();

    if (req->method == HTTP_GET) {
        cJSON *json = cJSON_CreateObject();
        cJSON_AddBoolToObject(json, "pending", reset_handler_is_pending());
        cJSON_AddNumberToObject(json, "remaining_s", reset_handler_remaining_s());
        const char *phase = "idle";
        switch (reset_handler_get_state()) {
            case RESET_STATE_IDLE:      phase = "idle";      break;
            case RESET_STATE_CONFIRM1:  phase = "confirm1";  break;
            case RESET_STATE_CONFIRM2:  phase = "confirm2";  break;
            case RESET_STATE_COUNTDOWN: phase = "countdown"; break;
            case RESET_STATE_ERASING:   phase = "erasing";   break;
            case RESET_STATE_REBOOTING: phase = "rebooting"; break;
        }
        cJSON_AddStringToObject(json, "phase", phase);
        return send_json_resp(req, json, "200 OK");
    }

    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *action_item = cJSON_GetObjectItem(json, "action");
    if (!cJSON_IsString(action_item)) {
        cJSON_Delete(json);
        return send_error(req, "missing_action", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    const char *action = action_item->valuestring;
    cJSON_Delete(json);

    esp_err_t err = ESP_ERR_INVALID_ARG;
    const char *status_msg = "unknown_action";

    if (strcmp(action, "factory") == 0) {
        if (reset_handler_get_state() == RESET_STATE_CONFIRM1) {
            err = reset_handler_confirm();
            status_msg = "confirmed";
        } else {
            err = reset_handler_start();
            status_msg = "initiated";
        }
    } else if (strcmp(action, "confirm") == 0) {
        err = reset_handler_confirm();
        status_msg = "confirmed";
    } else if (strcmp(action, "abort") == 0) {
        err = reset_handler_abort();
        status_msg = "aborted";
    }

    if (err != ESP_OK) {
        return send_error(req, "action_failed", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", status_msg);
    cJSON_AddStringToObject(resp, "action", action);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t config_monitor_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *mo = cJSON_GetObjectItem(json, "monitor_only");
    if (!cJSON_IsBool(mo)) {
        cJSON_Delete(json);
        return send_error(req, "missing_monitor_only", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    /* @requirement RF-WEB-006 Escrita de configuração exige validação. */
    cmd_validation_t cv = command_validator_can_set_config(&g_gs, "monitor_only_mode");
    if (!cv.allowed) {
        cJSON_Delete(json);
        return send_error(req, cv.error_code ? cv.error_code : "forbidden",
                          ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }
    bool val = cJSON_IsTrue(mo);
    g_gs.monitor_only_mode = val;
    config_set_monitor_only(val);
    cJSON_Delete(json);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "monitor_updated");
    cJSON_AddBoolToObject(resp, "monitor_only", val);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t plug_presets_list_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *arr = cJSON_CreateArray();
    for (uint8_t i = 0; i < plug_preset_count(); i++) {
        const plug_preset_t *p = plug_preset_get(i);
        if (!p) continue;
        cJSON *o = cJSON_CreateObject();
        cJSON_AddNumberToObject(o, "id", (int)p->id);
        cJSON_AddStringToObject(o, "name", p->name);
        cJSON_AddStringToObject(o, "icon", p->icon);
        cJSON_AddNumberToObject(o, "type", (int)p->type);
        cJSON_AddItemToArray(arr, o);
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "presets", arr);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t plug_preset_set_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

    cJSON *id_item = cJSON_GetObjectItem(json, "id");
    cJSON *preset_item = cJSON_GetObjectItem(json, "preset_id");
    if (!cJSON_IsNumber(id_item) || !cJSON_IsNumber(preset_item)) {
        cJSON_Delete(json);
        return send_error(req, "missing_fields", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    int plug_id = id_item->valueint;
    int preset_id = preset_item->valueint;
    if (plug_id < 3 || plug_id > 10) {
        cJSON_Delete(json);
        return send_error(req, "invalid_plug", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    cmd_validation_t cv = command_validator_can_set_config(&g_gs, "plug_preset");
    if (!cv.allowed) {
        cJSON_Delete(json);
        return send_error(req, cv.error_code ? cv.error_code : "forbidden",
                          ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    esp_err_t err = plug_manager_apply_preset((plug_id_t)plug_id, (plug_preset_id_t)preset_id);
    cJSON_Delete(json);
    if (err != ESP_OK) {
        return send_error(req, "preset_failed", ERR_INTERNAL, 500, "500 Internal Server Error");
    }

    audit_log_event(AUDIT_CONFIG_CHANGE, "plug preset via API");
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "preset_applied");
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t auth_recovery_handler(httpd_req_t *req)
{
    if (!g_gs.maintenance_mode) {
        return send_error(req, "maintenance_required", ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }
    esp_err_t err = auth_recovery_sd_process(true);
    if (err == ESP_OK) {
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddStringToObject(resp, "status", "recovery_ok");
        return send_json_resp(req, resp, "200 OK");
    }
    if (err == ESP_ERR_INVALID_ARG) {
        return send_error(req, "invalid_recovery_token", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    return send_error(req, "recovery_failed", ERR_INTERNAL, 500, "500 Internal Server Error");
}

static esp_err_t catch_all_handler(httpd_req_t *req)
{
    return send_error(req, "not_found", ERR_NOT_FOUND, 404, "404 Not Found");
}

static httpd_uri_t g_uris[] = {
    { .uri = "/api/v1/auth/login",    .method = HTTP_POST, .handler = login_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/auth/logout",   .method = HTTP_POST, .handler = logout_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/auth/password", .method = HTTP_POST, .handler = auth_password_handler, .user_ctx = NULL },
    { .uri = "/api/v1/auth/recovery", .method = HTTP_POST, .handler = auth_recovery_handler, .user_ctx = NULL },
    { .uri = "/api/v1/status",        .method = HTTP_GET,  .handler = status_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/state",         .method = HTTP_GET,  .handler = state_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/health",        .method = HTTP_GET,  .handler = health_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/plugs",         .method = HTTP_GET,  .handler = plugs_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/plugs",         .method = HTTP_POST, .handler = plug_set_handler,      .user_ctx = NULL },
    { .uri = "/api/v1/plugs/presets", .method = HTTP_GET,  .handler = plug_presets_list_handler, .user_ctx = NULL },
    { .uri = "/api/v1/plugs/preset",  .method = HTTP_POST, .handler = plug_preset_set_handler,   .user_ctx = NULL },
    { .uri = "/api/v1/plugs/mode",    .method = HTTP_POST, .handler = plug_mode_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/sensors",       .method = HTTP_GET,  .handler = sensors_handler,       .user_ctx = NULL },
    { .uri = "/api/v1/energy",        .method = HTTP_GET,  .handler = energy_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/alerts",        .method = HTTP_GET,  .handler = alerts_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/alerts",        .method = HTTP_POST, .handler = alert_ack_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/config",        .method = HTTP_GET,  .handler = config_get_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/config",        .method = HTTP_POST, .handler = config_set_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/config/export", .method = HTTP_GET,  .handler = config_export_handler,    .user_ctx = NULL },
    { .uri = "/api/v1/config/import", .method = HTTP_POST, .handler = config_import_handler,    .user_ctx = NULL },
    { .uri = "/api/v1/config/monitor", .method = HTTP_POST, .handler = config_monitor_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/command",       .method = HTTP_POST, .handler = command_handler,       .user_ctx = NULL },
    { .uri = "/api/v1/calibrate",     .method = HTTP_GET,  .handler = calibrate_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/calibrate",     .method = HTTP_POST, .handler = calibrate_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/feed",          .method = HTTP_POST, .handler = feed_handler,          .user_ctx = NULL },
    { .uri = "/api/v1/system",        .method = HTTP_GET,  .handler = system_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/log",           .method = HTTP_GET,  .handler = log_handler,           .user_ctx = NULL },
    { .uri = "/api/v1/wizard",        .method = HTTP_GET,  .handler = wizard_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/wizard",        .method = HTTP_POST, .handler = wizard_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/reset",         .method = HTTP_GET,  .handler = reset_api_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/reset",         .method = HTTP_POST, .handler = reset_api_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/*",             .method = HTTP_GET,  .handler = catch_all_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/*",             .method = HTTP_POST, .handler = catch_all_handler,     .user_ctx = NULL },
};

esp_err_t api_rest_init(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.max_uri_handlers = 36;
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar servidor HTTP: %s", esp_err_to_name(err));
        return err;
    }
    for (int i = 0; i < sizeof(g_uris) / sizeof(g_uris[0]); i++) {
        if (httpd_register_uri_handler(s_server, &g_uris[i]) != ESP_OK) {
            ESP_LOGW(TAG, "Falha ao registrar %s", g_uris[i].uri);
        }
    }
    ESP_LOGI(TAG, "API /api/v1 iniciada na porta 80");
    return ESP_OK;
}

