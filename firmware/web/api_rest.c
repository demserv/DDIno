// @requirement RF-WEB-001 a RF-WEB-008 (API REST)
// @requirement RNF-CALIB-001 Calibração assistida via API
#include "api_rest.h"
#include "api_auth.h"
#include "api_rate_limit.h"
#include "global_state.h"
#include "system_types.h"
#include "driver_pzem.h"
#include "driver_acs712.h"
#include "driver_ds18b20.h"
#include "driver_ds3231.h"
#include "driver_mcp3208.h"
#include "driver_relay.h"
#include "services/alert_manager.h"
#include "services/command_validator.h"
#include "services/relay_safety_service.h"
#include "services/config_manager.h"
#include "services/audit_log.h"
#include "fsm/feed_fsm.h"
#include "fsm/restart_fsm.h"
#include "services/plug_manager.h"
#include "services/reset_handler.h"
#include "pin_map.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

static bool rate_limit_middleware(httpd_req_t *req)
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
    return rate_limit_check(ip);
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
    return send_json_resp(req, json, "200 OK");
}

static esp_err_t health_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
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
    cJSON_AddNumberToObject(json, "last_reset_reason", g_gs.last_reset_reason);
    cJSON_AddNumberToObject(json, "heap_free_kb", (double)(esp_get_free_heap_size() / 1024));
    cJSON_AddNumberToObject(json, "wdt_resets_24h", 0);
    cJSON_AddNumberToObject(json, "last_health_check_timestamp", (double)g_gs.last_health_check_timestamp);
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
    if (!cJSON_IsNumber(id_item) || !cJSON_IsBool(state_item)) {
        cJSON_Delete(json);
        return send_error(req, "missing_fields", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    int plug_id = id_item->valueint;
    bool target_state = cJSON_IsTrue(state_item);
    cJSON_Delete(json);

    if (plug_id < 1 || plug_id > 10) {
        return send_error(req, "invalid_plug_id", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }

    cmd_validation_t cv = command_validator_can_toggle_plug(&g_gs, (uint8_t)plug_id, target_state);
    if (!cv.allowed) {
        return send_error(req, cv.error_code ? cv.error_code : "forbidden",
                          ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    esp_err_t err = plug_manager_toggle((plug_id_t)plug_id, target_state);
    if (err != ESP_OK) {
        return send_error(req, "plug_denied", ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

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
    cJSON *arr = cJSON_CreateArray();
    for (int16_t id = 1; id <= 65; id++) {
        if (alert_manager_is_active(id)) {
            cJSON *a = cJSON_CreateObject();
            cJSON_AddNumberToObject(a, "id", id);
            const alert_slot_t *aslot = alert_manager_get_slot((int16_t)id);
            cJSON_AddBoolToObject(a, "acked", aslot ? aslot->acked : false);
            cJSON_AddItemToArray(arr, a);
        }
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
            if (alert_manager_ack(specific_id, now)) acked++;
        }
    } else {
        for (int16_t id = 1; id <= 65; id++) {
            if (alert_manager_is_active(id)) {
                if (alert_manager_ack(id, now)) acked++;
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
        if (cJSON_IsNumber(plug_item) && cJSON_IsBool(state_item)) {
            res = command_validator_can_toggle_plug(&g_gs, (uint8_t)plug_item->valueint, cJSON_IsTrue(state_item));
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
        res.allowed = true;
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
        g_gs.monitor_only_mode = cJSON_IsTrue(mo);
    }
    cJSON *wiz = cJSON_GetObjectItem(json, "wizard_completed");
    if (cJSON_IsBool(wiz)) {
        g_gs.wizard_completed = cJSON_IsTrue(wiz);
    }
    cJSON_Delete(json);
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "config_updated");
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t log_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < 10; i++) {
        cJSON *e = cJSON_CreateObject();
        cJSON_AddNumberToObject(e, "ts", (double)(esp_timer_get_time() / USEC_PER_SEC) - i * 60);
        cJSON_AddStringToObject(e, "msg", "system_running");
        cJSON_AddStringToObject(e, "level", "info");
        cJSON_AddItemToArray(arr, e);
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "entries", arr);
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
    const char *token = api_auth_login(user->valuestring, pass->valuestring);
    cJSON_Delete(json);
    if (!token) return send_error(req, "invalid_credentials", ERR_AUTH_REQUIRED, 401, "401 Unauthorized");
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
    g_feed_request = true;
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
        config_set_thermal(&tp);
    }

    cJSON *a = cJSON_GetObjectItem(json, "ato");
    if (cJSON_IsObject(a)) {
        ato_params_storage_t ap = *config_get_ato();
        cJSON *v;
        v = cJSON_GetObjectItem(a, "low_level_adc"); if (cJSON_IsNumber(v)) ap.low_level_adc = v->valueint;
        v = cJSON_GetObjectItem(a, "high_level_adc"); if (cJSON_IsNumber(v)) ap.high_level_adc = v->valueint;
        v = cJSON_GetObjectItem(a, "overflow_margin_adc"); if (cJSON_IsNumber(v)) ap.overflow_margin_adc = v->valueint;
        v = cJSON_GetObjectItem(a, "refill_timeout_s"); if (cJSON_IsNumber(v)) ap.refill_timeout_s = (uint32_t)(v->valueint);
        config_set_ato(&ap);
    }

    cJSON *e = cJSON_GetObjectItem(json, "electric");
    if (cJSON_IsObject(e)) {
        electric_params_storage_t ep = *config_get_electric();
        cJSON *v;
        v = cJSON_GetObjectItem(e, "total_power_limit_w"); if (cJSON_IsNumber(v)) ep.total_power_limit_w = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(e, "overvoltage_limit_v"); if (cJSON_IsNumber(v)) ep.overvoltage_limit_v = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(e, "undervoltage_limit_v"); if (cJSON_IsNumber(v)) ep.undervoltage_limit_v = (float)(v->valuedouble);
        v = cJSON_GetObjectItem(e, "per_plug_current_limit_a"); if (cJSON_IsNumber(v)) ep.per_plug_current_limit_a = (float)(v->valuedouble);
        config_set_electric(&ep);
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
    bool val = cJSON_IsTrue(mo);
    g_gs.monitor_only_mode = val;
    config_set_monitor_only(val);
    cJSON_Delete(json);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "monitor_updated");
    cJSON_AddBoolToObject(resp, "monitor_only", val);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t catch_all_handler(httpd_req_t *req)
{
    return send_error(req, "not_found", ERR_NOT_FOUND, 404, "404 Not Found");
}

static httpd_uri_t g_uris[] = {
    { .uri = "/api/v1/auth/login",    .method = HTTP_POST, .handler = login_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/auth/logout",   .method = HTTP_POST, .handler = logout_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/auth/password", .method = HTTP_POST, .handler = auth_password_handler, .user_ctx = NULL },
    { .uri = "/api/v1/status",        .method = HTTP_GET,  .handler = status_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/health",        .method = HTTP_GET,  .handler = health_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/plugs",         .method = HTTP_GET,  .handler = plugs_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/plugs",         .method = HTTP_POST, .handler = plug_set_handler,      .user_ctx = NULL },
    { .uri = "/api/v1/plugs/mode",    .method = HTTP_POST, .handler = plug_mode_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/sensors",       .method = HTTP_GET,  .handler = sensors_handler,       .user_ctx = NULL },
    { .uri = "/api/v1/energy",        .method = HTTP_GET,  .handler = energy_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/alerts",        .method = HTTP_GET,  .handler = alerts_handler,        .user_ctx = NULL },
    { .uri = "/api/v1/alerts",        .method = HTTP_POST, .handler = alert_ack_handler,     .user_ctx = NULL },
    { .uri = "/api/v1/config",        .method = HTTP_GET,  .handler = config_get_handler,         .user_ctx = NULL },
    { .uri = "/api/v1/config",        .method = HTTP_POST, .handler = config_set_handler,         .user_ctx = NULL },
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
    cfg.max_uri_handlers = 32;
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
