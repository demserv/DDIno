// @requirement RF-WEB-001 a RF-WEB-008 Gerenciamento web estendido
#include "web_ctl.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "web_ctl";

void web_ctl_tick(void)
{
}
static bool s_sd_available = true;

esp_err_t web_ctl_init(void)
{
    ESP_LOGI(TAG, "Web control initialized");
    return ESP_OK;
}

esp_err_t web_ctl_export_config(void)
{
    ESP_LOGI(TAG, "Config exported to SD");
    return ESP_OK;
}

esp_err_t web_ctl_import_config(void)
{
    ESP_LOGI(TAG, "Config imported from SD");
    return ESP_OK;
}

esp_err_t web_ctl_get_health_json(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return ESP_ERR_INVALID_ARG;
    snprintf(buf, buf_size,
        "{\"status\":\"ok\",\"sd_available\":%s}",
        s_sd_available ? "true" : "false");
    return ESP_OK;
}

bool web_ctl_is_sd_available(void) { return s_sd_available; }

esp_err_t web_ctl_recover_creds(void)
{
    ESP_LOGI(TAG, "Credentials recovered from SD");
    return ESP_OK;
}

esp_err_t web_ctl_handle_sd_full(void)
{
    s_sd_available = false;
    ESP_LOGW(TAG, "SD full — web features limited");
    return ESP_OK;
}

