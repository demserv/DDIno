// @requirement RF-ATO-DIGITAL-001 ATO digital ON/OFF via MCP3208 #2 CH2, FSM 6 estados
#include "ato_service.h"

#include "esp_log.h"

#include "ato_fsm.h"
#include "config_manager.h"
#include "driver_mcp3208.h"
#include "hardware_config.h"
#include "pin_map.h"

static const char *TAG = "ato_svc";
static ato_fsm_t s_fsm;
static const ato_output_t *s_fsm_out = NULL;
static bool s_pump_on_pub = false;
static bool s_overflow_pub = false;
static bool s_digital_high = false;

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
    ESP_LOGI(TAG, "ATO service initialized (enabled=%d digital=%d low=%ld high=%ld)",
             cfg->enabled, cfg->digital_mode,
             (long)cfg->low_level_adc, (long)cfg->high_level_adc);
    return ESP_OK;
}

void ato_service_publish(bool pump_on, bool overflow)
{
    s_pump_on_pub = pump_on;
    s_overflow_pub = overflow;
}

static esp_err_t read_raw_adc(int32_t *raw_out)
{
    if (!raw_out) return ESP_ERR_INVALID_ARG;
    uint16_t adc = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &adc);
    if (err != ESP_OK) {
        *raw_out = -1;
        return err;
    }
    const calibration_params_storage_t *cal = config_get_calibration();
    int32_t raw = (int32_t)adc - cal->ato_zero_offset_adc;
    if (raw < 0) raw = 0;
    if (raw > (int32_t)HW_ADC_MAX_COUNT) raw = (int32_t)HW_ADC_MAX_COUNT;
    *raw_out = raw;
    return ESP_OK;
}

esp_err_t ato_service_get_level_adc(int32_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;

    int32_t raw = 0;
    esp_err_t err = read_raw_adc(&raw);
    if (err != ESP_OK) return err;

    const ato_params_storage_t *cfg = config_get_ato();
    if (cfg->digital_mode) {
        if (raw >= cfg->high_level_adc) {
            s_digital_high = true;
            *out = cfg->high_level_adc;
        } else if (raw <= cfg->low_level_adc) {
            s_digital_high = false;
            *out = cfg->low_level_adc - (int32_t)HW_ATO_ADC_HYSTERESIS;
        } else {
            *out = s_digital_high ? cfg->high_level_adc
                                  : (cfg->low_level_adc - (int32_t)HW_ATO_ADC_HYSTERESIS);
        }
    } else {
        *out = raw;
    }
    return ESP_OK;
}

esp_err_t ato_service_is_pump_on(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_pump_on_pub;
    return ESP_OK;
}

esp_err_t ato_service_is_overflow(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_overflow_pub;
    return ESP_OK;
}

esp_err_t ato_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}
