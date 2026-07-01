// @requirement RF-CONF-001 a RF-CONF-020 Gerenciamento estendido de configuração
#include "conf_ctl.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"

#include "storage_manager.h"

static const char *TAG = "conf_ctl";
static conf_ctl_profile_t s_profile = CONF_PROFILE_DEFAULT;
static conf_ctl_config_t s_saved;

static const conf_ctl_config_t s_defaults = {
    .network = { "", "", true, "0.0.0.0", "0.0.0.0", "0.0.0.0" },
    .sensor  = { 18.0f, 30.0f, 35.0f },
    .actuator = { 100, 100, true },
    .alert   = { true, 300 /* 300s ack timeout */, 1000 /* 1000ms spam cooldown */ },
    .display = { 80, true, 300 },
    .log     = { LOG_LEVEL_INFO, true, 512 /* 512KB max log file */ },
    .ota     = { false, "", 24 /* 24h check interval */ },
    .datetime = { "America/Sao_Paulo", -3 /* UTC-3 */, true, "pool.ntp.org" },
    .locale  = { "pt-BR", 'C' }
};

const char *conf_ctl_get_name(void) { return "conf_ctl"; }

esp_err_t conf_ctl_init(void)
{
    s_profile = CONF_PROFILE_DEFAULT;
    memcpy(&s_saved, &s_defaults, sizeof(s_saved));
    ESP_LOGI(TAG, "Config control initialized");
    return ESP_OK;
}

esp_err_t conf_ctl_load(conf_ctl_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;
    *cfg = s_saved;
    return ESP_OK;
}

esp_err_t conf_ctl_save(const conf_ctl_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;
    s_saved = *cfg;
    ESP_LOGI(TAG, "Configuration saved");
    return ESP_OK;
}

esp_err_t conf_ctl_validate(const conf_ctl_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;
    if (cfg->sensor.temp_min_c >= cfg->sensor.temp_max_c) return ESP_ERR_INVALID_ARG;
    if (cfg->locale.temp_unit != 'C' && cfg->locale.temp_unit != 'F') return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}

esp_err_t conf_ctl_rollback(void)
{
    memcpy(&s_saved, &s_defaults, sizeof(s_saved));
    ESP_LOGW(TAG, "Configuration rolled back to defaults");
    return ESP_OK;
}

esp_err_t conf_ctl_export_json(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return ESP_ERR_INVALID_ARG;
    snprintf(buf, buf_size,
        "{\"temp_min\":%.1f,\"temp_max\":%.1f,\"brightness\":%"PRIu32",\"sleep_timeout\":%"PRIu32",\"timezone\":\"%s\"}",
        s_saved.sensor.temp_min_c, s_saved.sensor.temp_max_c,
        (uint32_t)s_saved.display.brightness, (uint32_t)s_saved.display.sleep_timeout_s,
        s_saved.datetime.timezone);
    return ESP_OK;
}

esp_err_t conf_ctl_import_json(const char *json)
{
    (void)json;
    ESP_LOGW(TAG, "JSON import not fully implemented");
    return ESP_OK;
}

esp_err_t conf_ctl_set_profile(conf_ctl_profile_t profile)
{
    s_profile = profile;
    return ESP_OK;
}

conf_ctl_profile_t conf_ctl_get_profile(void) { return s_profile; }

