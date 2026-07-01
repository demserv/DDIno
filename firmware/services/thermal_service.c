// @requirement RF-THERMAL-001..010 Gerenciamento térmico
#include "thermal_service.h"

#include "esp_log.h"

#include "temp_filter.h"
#include "thermal_fsm.h"

static const char *TAG = "thermal_svc";
static thermal_fsm_t s_fsm;
static const thermal_output_t *s_fsm_out = NULL;
static float s_setpoint = 25.0f;
static float s_current_c = 0.0f;
static bool s_sample_valid = false;
static bool s_heating_pub = false;
static bool s_cooling_pub = false;

esp_err_t thermal_service_init(void)
{
    thermal_params_t p = { .temp_normal_c = 25.0f, .temp_critical_c = 32.0f, .temp_extreme_c = 38.0f };
    thermal_fsm_init(&s_fsm, &p);
    s_fsm_out = thermal_fsm_get_output(&s_fsm);
    ESP_LOGI(TAG, "Thermal service initialized");
    return ESP_OK;
}

void thermal_service_publish(float current_c, bool sample_valid, bool heating, bool cooling)
{
    s_current_c = current_c;
    s_sample_valid = sample_valid;
    s_heating_pub = heating;
    s_cooling_pub = cooling;
}

esp_err_t thermal_service_get_setpoint(float *out_c)
{
    if (!out_c) return ESP_ERR_INVALID_ARG;
    *out_c = s_setpoint;
    return ESP_OK;
}

esp_err_t thermal_service_set_setpoint(float c)
{
    s_setpoint = c;
    ESP_LOGI(TAG, "Setpoint set to %.1fC", c);
    return ESP_OK;
}

esp_err_t thermal_service_get_current(float *out_c)
{
    if (!out_c) return ESP_ERR_INVALID_ARG;
    if (!s_sample_valid) return ESP_ERR_INVALID_STATE;
    *out_c = s_current_c;
    return ESP_OK;
}

esp_err_t thermal_service_is_heating(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_heating_pub;
    return ESP_OK;
}

esp_err_t thermal_service_is_cooling(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_cooling_pub;
    return ESP_OK;
}

esp_err_t thermal_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}

