// @requirement RF-ALERT-007 a RF-ALERT-020 Gerenciamento estendido de alertas
#include "alm_ctl.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "alm_ctl";
#define ALM_CTL_MAX_ACTIVE 16
static alm_ctl_entry_t s_active[ALM_CTL_MAX_ACTIVE];
static int s_active_count = 0;
static bool s_silenced = false;
static uint32_t s_silence_until = 0;
static uint32_t s_cooldown_ms = 1000;
static uint32_t s_last_id = 0;

esp_err_t alm_ctl_init(void)
{
    s_active_count = 0;
    s_silenced = false;
    s_silence_until = 0;
    s_last_id = 0;
    ESP_LOGI(TAG, "Alert control initialized");
    return ESP_OK;
}

esp_err_t alm_ctl_raise(const char *code, alm_ctl_severity_t severity, const char *msg)
{
    if (!code || !msg) return ESP_ERR_INVALID_ARG;
    if (s_silenced) return ESP_OK;
    if (s_active_count >= ALM_CTL_MAX_ACTIVE) return ESP_FAIL;
    int idx = s_active_count;
    s_active[idx].id = ++s_last_id;
    s_active[idx].severity = severity;
    strncpy(s_active[idx].code, code, sizeof(s_active[idx].code) - 1);
    strncpy(s_active[idx].message, msg, sizeof(s_active[idx].message) - 1);
    s_active[idx].timestamp_s = 0;
    s_active[idx].acked = false;
    s_active_count++;
    ESP_LOGW(TAG, "Alert raised: %s — %s", code, msg);
    return ESP_OK;
}

esp_err_t alm_ctl_ack(uint32_t id)
{
    for (int i = 0; i < s_active_count; i++) {
        if (s_active[i].id == id) {
            s_active[i].acked = true;
            for (int j = i; j < s_active_count - 1; j++) s_active[j] = s_active[j + 1];
            s_active_count--;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t alm_ctl_silence(uint32_t timeout_s)
{
    s_silenced = true;
    s_silence_until = timeout_s;
    return ESP_OK;
}

esp_err_t alm_ctl_unsilence(void) { s_silenced = false; s_silence_until = 0; return ESP_OK; }
bool alm_ctl_is_silenced(void) { return s_silenced; }

int alm_ctl_get_active_count(void) { return s_active_count; }

alm_ctl_entry_t *alm_ctl_get_active(int index)
{
    if (index < 0 || index >= s_active_count) return NULL;
    return &s_active[index];
}

esp_err_t alm_ctl_dedup_cooldown(uint32_t ms) { s_cooldown_ms = ms; return ESP_OK; }

