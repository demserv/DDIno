// @requirement RF-ATO-001..010 Gerenciamento ATO
// @requirement RF-ATO-DIGITAL-001 ATO digital ON/OFF via MCP3208 #2 CH2, FSM 6 estados
#include "ato_service.h"
#include "ato_fsm.h"
#include "config_manager.h"
#include "driver_mcp3208.h"
#include "pin_map.h"
#include "esp_log.h"

static const char *TAG = "ato_svc";
static ato_fsm_t s_fsm;
static const ato_output_t *s_fsm_out = NULL;

esp_err_t ato_service_init(void)
{
    const ato_params_storage_t *cfg = config_get_ato();
    ato_params_t p = {
        .enabled             = cfg->enabled,
        .low_level_adc       = cfg->low_level_adc,
        .high_level_adc      = cfg->high_level_adc,
        .overflow_margin_adc = cfg->overflow_margin_adc,
        .refill_timeout_s    = cfg->refill_timeout_s
    };
    ato_fsm_init(&s_fsm, &p);
    s_fsm_out = ato_fsm_get_output(&s_fsm);
    ESP_LOGI(TAG, "ATO service initialized (enabled=%d, low=%ld, high=%ld)",
             cfg->enabled, (long)cfg->low_level_adc, (long)cfg->high_level_adc);
    return ESP_OK;
}

esp_err_t ato_service_get_level_adc(int32_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    uint16_t adc = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &adc);
    if (err != ESP_OK) {
        *out = -1;
        return err;
    }
    *out = (int32_t)adc;
    return ESP_OK;
}

esp_err_t ato_service_is_pump_on(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (s_fsm_out && s_fsm_out->pump_request_on);
    return ESP_OK;
}

esp_err_t ato_service_is_overflow(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (s_fsm_out && s_fsm_out->state == ATO_STATE_OVERFLOW);
    return ESP_OK;
}

esp_err_t ato_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}
