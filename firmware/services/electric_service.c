// @requirement RF-ELECTRIC-BIVOLT-001 Gerenciamento elétrico
#include "electric_service.h"
#include "electric_fsm.h"
#include "esp_log.h"

static const char *TAG = "electric_svc";
static electric_fsm_t s_fsm;
static const electric_output_t *s_fsm_out = NULL;

esp_err_t electric_service_init(void)
{
    electric_params_t p = { .total_power_limit_w = 1500, .per_plug_current_limit_a = 10.0f };
    electric_fsm_init(&s_fsm, &p);
    s_fsm_out = electric_fsm_get_output(&s_fsm);
    ESP_LOGI(TAG, "Electric service initialized");
    return ESP_OK;
}

esp_err_t electric_service_get_power_w(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = 0.0f;
    return ESP_OK;
}

esp_err_t electric_service_get_current_a(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = 0.0f;
    return ESP_OK;
}

esp_err_t electric_service_get_voltage_v(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = 127.0f;
    return ESP_OK;
}

esp_err_t electric_service_is_overload(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = false;
    return ESP_OK;
}

esp_err_t electric_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}
