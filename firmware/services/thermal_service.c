// @requirement RF-THERMAL-001..010 Gerenciamento térmico
#include "thermal_service.h"

#include "esp_log.h"

#include "temp_filter.h"
#include "thermal_fsm.h"

static const char *TAG = "thermal_svc";
static thermal_fsm_t s_fsm;
static const thermal_output_t *s_fsm_out = NULL;
static float s_setpoint = 25.0f;

esp_err_t thermal_service_init(void)
{
    thermal_params_t p = { .temp_normal_c = 25.0f, .temp_critical_c = 32.0f, .temp_extreme_c = 38.0f };
    thermal_fsm_init(&s_fsm, &p);
    s_fsm_out = thermal_fsm_get_output(&s_fsm);
    ESP_LOGI(TAG, "Thermal service initialized");
    return ESP_OK;
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

/* @requirement RF-THERMAL-001 leitura via FSM (sem hardcode 0) */
esp_err_t thermal_service_get_current(float *out_c)
{
    if (!out_c) return ESP_ERR_INVALID_ARG;
    float v;
    if (thermal_fsm_get_last_valid_temp(&s_fsm, &v)) {
        *out_c = v;
        return ESP_OK;
    }
    *out_c = 0.0f;
    return ESP_ERR_INVALID_STATE;
}

esp_err_t thermal_service_is_heating(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (s_fsm_out && s_fsm_out->request_heater_on);
    return ESP_OK;
}

esp_err_t thermal_service_is_cooling(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (s_fsm_out && s_fsm_out->request_cooler_on);
    return ESP_OK;
}

esp_err_t thermal_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}

