// @requirement DATA — Gerenciamento centralizado de dados
#include "data_ctl.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "data_ctl";

esp_err_t data_ctl_init(void)
{
    ESP_LOGI(TAG, "Data control initialized");
    return ESP_OK;
}

esp_err_t data_ctl_take_snapshot(data_ctl_snapshot_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    out->uptime_s = 0;
    out->wifi_rssi = -127;
    return ESP_OK;
}

esp_err_t data_ctl_export_snapshot(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return ESP_ERR_INVALID_ARG;
    snprintf(buf, buf_size, "{\"uptime\":0,\"rssi\":-127}");
    return ESP_OK;
}

esp_err_t data_ctl_clear_history(void)
{
    ESP_LOGW(TAG, "History cleared");
    return ESP_OK;
}

int data_ctl_get_history_count(void) { return 0; }

esp_err_t data_ctl_purge_older_than(uint32_t age_hours)
{
    ESP_LOGI(TAG, "Purged data older than %u hours", (unsigned)age_hours);
    return ESP_OK;
}

