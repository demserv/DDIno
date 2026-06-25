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
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "pin_map.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "api_rest";

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

static httpd_handle_t s_server = NULL;

#define ERR_AUTH_REQUIRED    1001
#define ERR_INVALID_TOKEN    1002
#define ERR_RATE_LIMITED     1003
#define ERR_VALIDATION_ERROR 2001
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
    return (strcmp(uri, "/api/v1/auth/login") == 0) || (strcmp(uri, "/api/v1/auth/logout") == 0);
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
    cJSON_AddStringToObject(json, "srs_version", g_gs.srs_version);
    cJSON_AddNumberToObject(json, "active_alm_categories", (double)g_gs.active_alm_categories);
    cJSON_AddNumberToObject(json, "last_reset_reason", g_gs.last_reset_reason);
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
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "id", i + 1);
        cJSON_AddBoolToObject(p, "state", relay_get((uint8_t)(i + 1)));
        cJSON_AddBoolToObject(p, "command_allowed", g_gs.system_state == SYSTEM_STATE_NORMAL || g_gs.system_state == SYSTEM_STATE_DEGRADED);
        float cur = 0;
        acs712_read_plug((uint8_t)(i + 1), &cur);
        cJSON_AddNumberToObject(p, "current_a", (double)cur);
        cJSON_AddItemToArray(arr, p);
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "plugs", arr);
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
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "alerts", arr);
    cJSON_AddNumberToObject(resp, "count", g_gs.active_alerts_count);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "wizard_completed", g_gs.wizard_completed);
    cJSON_AddBoolToObject(json, "monitor_only_mode", g_gs.monitor_only_mode);
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

    bool effective_on = false;
    relay_apply_request_t rar = {
        .system_state = g_gs.system_state,
        .monitor_only_mode = g_gs.monitor_only_mode,
        .plug_id = (uint8_t)plug_id,
        .desired_on = target_state,
        .critical_manual_confirmed = false,
        .plug_is_critical_role = (plug_id == 1 || plug_id == 2)
    };
    relay_apply_result_t rs = relay_safety_compute(&rar, &effective_on);
    if (rs != RELAY_APPLY_OK) {
        return send_error(req, "safety_denied", ERR_SAFE_MODE_ACTIVE, 403, "403 Forbidden");
    }

    esp_err_t err = relay_set((uint8_t)plug_id, effective_on);
    if (err != ESP_OK) {
        return send_error(req, "relay_failure", ERR_INTERNAL, 500, "500 Internal Server Error");
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "id", plug_id);
    cJSON_AddBoolToObject(resp, "state", effective_on);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t alert_ack_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    (void)req;
    uint64_t now = esp_timer_get_time() / 1000000ULL;
    int acked = 0;
    for (int16_t id = 1; id <= 65; id++) {
        if (alert_manager_is_active(id)) {
            if (alert_manager_ack(id, now)) acked++;
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
    cJSON *plug_item = cJSON_GetObjectItem(json, "plug_id");
    cJSON *state_item = cJSON_GetObjectItem(json, "state");
    if (!cJSON_IsString(cmd_item)) {
        cJSON_Delete(json);
        return send_error(req, "missing_cmd", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    }
    const char *cmd = cmd_item->valuestring;
    cmd_validation_t res = { .allowed = false, .requires_double_confirmation = false, .error_code = "unknown_cmd" };
    if (strcmp(cmd, "toggle_plug") == 0 && cJSON_IsNumber(plug_item) && cJSON_IsBool(state_item)) {
        res = command_validator_can_toggle_plug(&g_gs, (uint8_t)plug_item->valueint, cJSON_IsTrue(state_item));
    }
    cJSON_Delete(json);
    if (!res.allowed) {
        return send_error(req, res.error_code ? res.error_code : "forbidden", ERR_VALIDATION_ERROR, 403, "403 Forbidden");
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "command_accepted");
    cJSON_AddStringToObject(resp, "cmd", cmd);
    return send_json_resp(req, resp, "200 OK");
}

static esp_err_t log_handler(httpd_req_t *req)
{
    AUTH_GUARD();
    cJSON *arr = cJSON_CreateArray();
    cJSON *e = cJSON_CreateObject();
    cJSON_AddNumberToObject(e, "ts", (double)(esp_timer_get_time() / 1000000ULL));
    cJSON_AddStringToObject(e, "msg", "system_running");
    cJSON_AddStringToObject(e, "level", "info");
    cJSON_AddItemToArray(arr, e);
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
    cJSON_AddNumberToObject(resp, "expires_in_s", 3600);
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
    char *body = read_body(req);
    if (!body) return send_error(req, "invalid_body", ERR_VALIDATION_ERROR, 400, "400 Bad Request");
    cJSON *json = cJSON_Parse(body);
    free(body);
    if (!json) return send_error(req, "invalid_json", ERR_VALIDATION_ERROR, 400, "400 Bad Request");

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

static esp_err_t catch_all_handler(httpd_req_t *req)
{
    return send_error(req, "not_found", ERR_NOT_FOUND, 404, "404 Not Found");
}

static httpd_uri_t g_uris[] = {
    { .uri = "/api/v1/auth/login",    .method = HTTP_POST, .handler = login_handler,    .user_ctx = NULL },
    { .uri = "/api/v1/auth/logout",   .method = HTTP_POST, .handler = logout_handler,   .user_ctx = NULL },
    { .uri = "/api/v1/status",        .method = HTTP_GET,  .handler = status_handler,   .user_ctx = NULL },
    { .uri = "/api/v1/health",        .method = HTTP_GET,  .handler = health_handler,   .user_ctx = NULL },
    { .uri = "/api/v1/plugs",         .method = HTTP_GET,  .handler = plugs_handler,    .user_ctx = NULL },
    { .uri = "/api/v1/plugs",         .method = HTTP_POST, .handler = plug_set_handler, .user_ctx = NULL },
    { .uri = "/api/v1/sensors",       .method = HTTP_GET,  .handler = sensors_handler,  .user_ctx = NULL },
    { .uri = "/api/v1/energy",        .method = HTTP_GET,  .handler = energy_handler,   .user_ctx = NULL },
    { .uri = "/api/v1/alerts",        .method = HTTP_GET,  .handler = alerts_handler,   .user_ctx = NULL },
    { .uri = "/api/v1/alerts",        .method = HTTP_POST, .handler = alert_ack_handler,.user_ctx = NULL },
    { .uri = "/api/v1/config",        .method = HTTP_GET,  .handler = config_get_handler,.user_ctx = NULL },
    { .uri = "/api/v1/config",        .method = HTTP_POST, .handler = config_set_handler,.user_ctx = NULL },
    { .uri = "/api/v1/command",       .method = HTTP_POST, .handler = command_handler,  .user_ctx = NULL },
    { .uri = "/api/v1/calibrate",     .method = HTTP_POST, .handler = calibrate_handler,.user_ctx = NULL },
    { .uri = "/api/v1/log",           .method = HTTP_GET,  .handler = log_handler,      .user_ctx = NULL },
    { .uri = "/api/v1/*",             .method = HTTP_GET,  .handler = catch_all_handler, .user_ctx = NULL },
    { .uri = "/api/v1/*",             .method = HTTP_POST, .handler = catch_all_handler, .user_ctx = NULL },
};

esp_err_t api_rest_init(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.max_uri_handlers = 24;
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
