// @requirement RF-ATO-001..010 Gerenciamento ATO
// @requirement RF-ATO-DIGITAL-001 ATO digital ON/OFF via MCP3208 #2 CH2, FSM 6 estados
// @requirement RF-DADOS-001 Fonte única de estado: a ato_fsm autoritativa vive no
// laço de controle (app_main). Este serviço NÃO mantém FSM própria; expõe os
// valores publicados pelo laço e a leitura direta do ADC de nível.
#include "ato_service.h"

#include "esp_log.h"

#include "config_manager.h"
#include "driver_mcp3208.h"
#include "pin_map.h"

static const char *TAG = "ato_svc";
static bool s_pump_on = false;
static bool s_overflow = false;

esp_err_t ato_service_init(void)
{
    const ato_params_storage_t *cfg = config_get_ato();
    s_pump_on = false;
    s_overflow = false;
    ESP_LOGI(TAG, "ATO service initialized (enabled=%d, low=%ld, high=%ld)",
             cfg->enabled, (long)cfg->low_level_adc, (long)cfg->high_level_adc);
    return ESP_OK;
}

void ato_service_publish(bool pump_on, bool overflow)
{
    s_pump_on = pump_on;
    s_overflow = overflow;
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
    *out = s_pump_on;
    return ESP_OK;
}

esp_err_t ato_service_is_overflow(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_overflow;
    return ESP_OK;
}

esp_err_t ato_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}
