/* @requirement RF-WEB-008 Export/import JSON de configuracao com CRC */
#include "services/config_export.h"
#include "services/config_manager.h"
#include "config_root.h"
#include "param_catalog.h"

#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "cfg_export";

static void root_from_current(config_root_t *root)
{
    memset(root, 0, sizeof(*root));
    snprintf(root->schema_version, sizeof(root->schema_version), "%s", CONFIG_ROOT_SCHEMA_VERSION);
    root->thermal     = *config_get_thermal();
    root->ato         = *config_get_ato();
    root->electric    = *config_get_electric();
    root->plug_limits = *config_get_plug_limits();
    root->restart     = *config_get_restart();
    root->feed        = *config_get_feed();
    root->security    = *config_get_security();
    root->antiflap    = *config_get_antiflap();
    root->selftest    = *config_get_selftest();
    root->system      = *config_get_system();
    root->calibration = *config_get_calibration();
    config_root_compute_crc(root);
}

static cJSON *thermal_to_json(const thermal_params_storage_t *p)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddNumberToObject(o, "temp_normal_c", p->temp_normal_c);
    cJSON_AddNumberToObject(o, "temp_critical_c", p->temp_critical_c);
    cJSON_AddNumberToObject(o, "temp_extreme_c", p->temp_extreme_c);
    cJSON_AddNumberToObject(o, "temp_min_c", p->temp_min_c);
    cJSON_AddNumberToObject(o, "temp_max_c", p->temp_max_c);
    cJSON_AddNumberToObject(o, "hysteresis_c", p->hysteresis_c);
    cJSON_AddBoolToObject(o, "extreme_enabled", p->extreme_enabled);
    return o;
}

static cJSON *root_to_json(const config_root_t *root)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "schema_version", root->schema_version);
    cJSON_AddNumberToObject(o, "crc32", (double)root->crc32);
    cJSON_AddItemToObject(o, "thermal", thermal_to_json(&root->thermal));
    cJSON *ato = cJSON_CreateObject();
    cJSON_AddBoolToObject(ato, "enabled", root->ato.enabled);
    cJSON_AddNumberToObject(ato, "low_level_adc", (double)root->ato.low_level_adc);
    cJSON_AddNumberToObject(ato, "high_level_adc", (double)root->ato.high_level_adc);
    cJSON_AddNumberToObject(ato, "overflow_margin_adc", (double)root->ato.overflow_margin_adc);
    cJSON_AddNumberToObject(ato, "refill_timeout_s", (double)root->ato.refill_timeout_s);
    cJSON_AddItemToObject(o, "ato", ato);
    cJSON *elec = cJSON_CreateObject();
    cJSON_AddNumberToObject(elec, "total_power_limit_w", root->electric.total_power_limit_w);
    cJSON_AddNumberToObject(elec, "per_plug_current_limit_a", root->electric.per_plug_current_limit_a);
    cJSON_AddNumberToObject(elec, "overvoltage_limit_v", root->electric.overvoltage_limit_v);
    cJSON_AddNumberToObject(elec, "undervoltage_limit_v", root->electric.undervoltage_limit_v);
    cJSON_AddNumberToObject(elec, "total_current_limit_a", root->electric.total_current_limit_a);
    cJSON_AddItemToObject(o, "electric", elec);
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddBoolToObject(sys, "wizard_completed", root->system.wizard_completed);
    cJSON_AddNumberToObject(sys, "mains_voltage", root->system.mains_voltage);
    cJSON_AddBoolToObject(sys, "monitor_only_mode", root->system.monitor_only_mode);
    cJSON_AddItemToObject(o, "system", sys);
    cJSON *pl = cJSON_CreateObject();
    cJSON_AddNumberToObject(pl, "min_on_time_s", (double)root->plug_limits.min_on_time_s);
    cJSON_AddNumberToObject(pl, "min_off_time_s", (double)root->plug_limits.min_off_time_s);
    cJSON_AddItemToObject(o, "plug_limits", pl);
    cJSON *feed = cJSON_CreateObject();
    cJSON_AddNumberToObject(feed, "feed_duration_min", (double)root->feed.feed_duration_min);
    cJSON_AddNumberToObject(feed, "feed_cooldown_min", (double)root->feed.feed_cooldown_min);
    cJSON_AddItemToObject(o, "feed", feed);
    cJSON *rst = cJSON_CreateObject();
    cJSON_AddNumberToObject(rst, "tempo_espera_religamento_s", (double)root->restart.tempo_espera_religamento_s);
    cJSON_AddNumberToObject(rst, "intervalo_religamento_s", (double)root->restart.intervalo_religamento_s);
    cJSON_AddNumberToObject(rst, "tempo_monitoramento_pos_relig_s", (double)root->restart.tempo_monitoramento_pos_relig_s);
    cJSON_AddItemToObject(o, "restart", rst);
    cJSON *sec = cJSON_CreateObject();
    cJSON_AddNumberToObject(sec, "session_timeout_min", (double)root->security.session_timeout_min);
    cJSON_AddNumberToObject(sec, "max_login_attempts", (double)root->security.max_login_attempts);
    cJSON_AddNumberToObject(sec, "login_block_duration_min", (double)root->security.login_block_duration_min);
    cJSON_AddBoolToObject(sec, "read_requires_auth", root->security.read_requires_auth);
    cJSON_AddItemToObject(o, "security", sec);
    cJSON *af = cJSON_CreateObject();
    cJSON_AddNumberToObject(af, "tempo_min_estabilizacao_s", (double)root->antiflap.tempo_min_estabilizacao_s);
    cJSON_AddNumberToObject(af, "janela_flap_s", (double)root->antiflap.janela_flap_s);
    cJSON_AddNumberToObject(af, "max_transicoes_flap", (double)root->antiflap.max_transicoes_flap);
    cJSON_AddNumberToObject(af, "cooldown_reentrada_s", (double)root->antiflap.cooldown_reentrada_s);
    cJSON_AddItemToObject(o, "antiflap", af);
    cJSON *st = cJSON_CreateObject();
    cJSON_AddNumberToObject(st, "selftest_timeout_ms", (double)root->selftest.selftest_timeout_ms);
    cJSON_AddItemToObject(o, "selftest", st);
    cJSON *cal = cJSON_CreateObject();
    cJSON_AddNumberToObject(cal, "ato_zero_offset_adc", (double)root->calibration.ato_zero_offset_adc);
    cJSON_AddNumberToObject(cal, "temp_offset_c", (double)root->calibration.temp_offset_c);
    cJSON_AddItemToObject(o, "calibration", cal);
    return o;
}

esp_err_t config_export_to_json(cJSON **out_json)
{
    if (!out_json) return ESP_ERR_INVALID_ARG;
    config_root_t root;
    root_from_current(&root);
    *out_json = root_to_json(&root);
    return *out_json ? ESP_OK : ESP_ERR_NO_MEM;
}

static bool parse_thermal(cJSON *j, thermal_params_storage_t *p)
{
    if (!j || !p) return false;
    cJSON *v;
    v = cJSON_GetObjectItem(j, "temp_normal_c");
    if (cJSON_IsNumber(v)) p->temp_normal_c = (float)v->valuedouble;
    v = cJSON_GetObjectItem(j, "temp_critical_c");
    if (cJSON_IsNumber(v)) p->temp_critical_c = (float)v->valuedouble;
    v = cJSON_GetObjectItem(j, "temp_min_c");
    if (cJSON_IsNumber(v)) p->temp_min_c = (float)v->valuedouble;
    v = cJSON_GetObjectItem(j, "temp_max_c");
    if (cJSON_IsNumber(v)) p->temp_max_c = (float)v->valuedouble;
    if (p->temp_min_c >= p->temp_max_c) return false;
    return true;
}

static bool json_to_root(cJSON *json, config_root_t *root, const config_root_t *base, uint32_t *expected_crc)
{
    if (!json || !root || !base) return false;
    *root = *base;
    cJSON *sv = cJSON_GetObjectItem(json, "schema_version");
    if (cJSON_IsString(sv)) {
        snprintf(root->schema_version, sizeof(root->schema_version), "%s", sv->valuestring);
    }
    cJSON *crc = cJSON_GetObjectItem(json, "crc32");
    if (cJSON_IsNumber(crc) && expected_crc) {
        *expected_crc = (uint32_t)crc->valuedouble;
    }
    cJSON *th = cJSON_GetObjectItem(json, "thermal");
    if (th && !parse_thermal(th, &root->thermal)) return false;
    cJSON *ato = cJSON_GetObjectItem(json, "ato");
    if (cJSON_IsObject(ato)) {
        cJSON *en = cJSON_GetObjectItem(ato, "enabled");
        if (cJSON_IsBool(en)) root->ato.enabled = cJSON_IsTrue(en);
        cJSON *v = cJSON_GetObjectItem(ato, "low_level_adc");
        if (cJSON_IsNumber(v)) root->ato.low_level_adc = (int32_t)v->valuedouble;
        v = cJSON_GetObjectItem(ato, "high_level_adc");
        if (cJSON_IsNumber(v)) root->ato.high_level_adc = (int32_t)v->valuedouble;
    }
    cJSON *elec = cJSON_GetObjectItem(json, "electric");
    if (cJSON_IsObject(elec)) {
        cJSON *v = cJSON_GetObjectItem(elec, "total_power_limit_w");
        if (cJSON_IsNumber(v)) root->electric.total_power_limit_w = (float)v->valuedouble;
        v = cJSON_GetObjectItem(elec, "per_plug_current_limit_a");
        if (cJSON_IsNumber(v)) root->electric.per_plug_current_limit_a = (float)v->valuedouble;
    }
    cJSON *sys = cJSON_GetObjectItem(json, "system");
    if (cJSON_IsObject(sys)) {
        cJSON *v = cJSON_GetObjectItem(sys, "monitor_only_mode");
        if (cJSON_IsBool(v)) root->system.monitor_only_mode = cJSON_IsTrue(v);
    }
    cJSON *sec = cJSON_GetObjectItem(json, "security");
    if (cJSON_IsObject(sec)) {
        cJSON *v = cJSON_GetObjectItem(sec, "read_requires_auth");
        if (cJSON_IsBool(v)) root->security.read_requires_auth = cJSON_IsTrue(v);
    }
    config_root_compute_crc(root);
    if (expected_crc != 0 && root->crc32 != expected_crc) {
        ESP_LOGW(TAG, "Import CRC mismatch: expected 0x%08lX computed 0x%08lX",
                 (unsigned long)expected_crc, (unsigned long)root->crc32);
        return false;
    }
    return config_root_validate(root);
}

esp_err_t config_import_from_json(cJSON *json, bool dry_run, bool *valid_out, uint32_t *crc_out)
{
    if (!json) return ESP_ERR_INVALID_ARG;
    config_root_t incoming;
    config_root_t backup;
    root_from_current(&backup);

    uint32_t expected_crc = 0;
    if (!json_to_root(json, &incoming, &backup, &expected_crc)) {
        if (valid_out) *valid_out = false;
        return ESP_ERR_INVALID_ARG;
    }
    if (valid_out) *valid_out = true;
    if (crc_out) *crc_out = incoming.crc32;

    if (dry_run) {
        ESP_LOGI(TAG, "Import preview OK crc=0x%08lX", (unsigned long)incoming.crc32);
        return ESP_OK;
    }

    esp_err_t err;
    err = config_set_thermal(&incoming.thermal); if (err) goto rollback;
    err = config_set_ato(&incoming.ato); if (err) goto rollback;
    err = config_set_electric(&incoming.electric); if (err) goto rollback;
    err = config_set_plug_limits(&incoming.plug_limits); if (err) goto rollback;
    err = config_set_restart(&incoming.restart); if (err) goto rollback;
    err = config_set_feed(&incoming.feed); if (err) goto rollback;
    err = config_set_security(&incoming.security); if (err) goto rollback;
    err = config_set_antiflap(&incoming.antiflap); if (err) goto rollback;
    err = config_set_selftest(&incoming.selftest); if (err) goto rollback;
    err = config_set_calibration(&incoming.calibration); if (err) goto rollback;
    if (incoming.system.monitor_only_mode != backup.system.monitor_only_mode) {
        config_set_monitor_only(incoming.system.monitor_only_mode);
    }
    err = config_save_all();
    if (err) goto rollback;
    ESP_LOGI(TAG, "Import committed crc=0x%08lX", (unsigned long)incoming.crc32);
    return ESP_OK;

rollback:
    config_set_thermal(&backup.thermal);
    config_set_ato(&backup.ato);
    config_set_electric(&backup.electric);
    config_set_plug_limits(&backup.plug_limits);
    config_set_restart(&backup.restart);
    config_set_feed(&backup.feed);
    config_set_security(&backup.security);
    config_set_antiflap(&backup.antiflap);
    config_set_selftest(&backup.selftest);
    config_set_calibration(&backup.calibration);
    config_set_monitor_only(backup.system.monitor_only_mode);
    config_save_all();
    ESP_LOGE(TAG, "Import failed, rolled back");
    return err;
}
