// @requirement RF-ATO-001..010 Gerenciamento ATO
#include "ato_service.h"
#include "ato_fsm.h"
#include "esp_log.h"

static const char *TAG = "ato_svc";
static ato_fsm_t s_fsm;
static const ato_output_t *s_fsm_out = NULL;

esp_err_t ato_service_init(void)
{
    ato_params_t p = { .enabled = true, .low_level_adc = 500, .high_level_adc = 3000, .overflow_margin_adc = 200 };
    ato_fsm_init(&s_fsm, &p);
    s_fsm_out = ato_fsm_get_output(&s_fsm);
    ESP_LOGI(TAG, "ATO service initialized");
    return ESP_OK;
}

esp_err_t ato_service_get_level_adc(int32_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = 0;
    return ESP_OK;
}

esp_err_t ato_service_is_pump_on(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (s_fsm_out && s_fsm_out->pump_on);
    return ESP_OK;
}

esp_err_t ato_service_is_overflow(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (s_fsm_out && s_fsm_out->overflow_detected);
    return ESP_OK;
}

esp_err_t ato_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}
