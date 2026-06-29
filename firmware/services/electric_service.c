#include "electric_service.h"

#include "esp_log.h"

#include "driver_acs712.h"
#include "driver_pzem.h"
#include "electric_fsm.h"

static const char *TAG = "electric_svc";
static electric_fsm_t s_fsm;
static const electric_output_t *s_fsm_out = NULL;
static pzem_data_t s_pzem;
static bool s_pzem_valid = false;

esp_err_t electric_service_init(void)
{
    electric_params_t p = { .total_power_limit_w = 1500, .per_plug_current_limit_a = 10.0f };
    electric_fsm_init(&s_fsm, &p);
    s_fsm_out = electric_fsm_get_output(&s_fsm);
    s_pzem_valid = (pzem_init() == ESP_OK);
    ESP_LOGI(TAG, "Electric service initialized (PZEM %s)", s_pzem_valid ? "OK" : "FAIL");
    return ESP_OK;
}

esp_err_t electric_service_get_power_w(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    esp_err_t err = pzem_read_all(&s_pzem);
    if (err == ESP_OK && s_pzem.valid) {
        *out = s_pzem.power_w;
    } else {
        *out = 0.0f;
        ESP_LOGW(TAG, "PZEM read failed, power=0");
    }
    return ESP_OK;
}

esp_err_t electric_service_get_current_a(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    float total = 0.0f;
    float cur;
    uint8_t ok = 0;
    for (uint8_t i = 1; i <= ACS712_CHANNEL_COUNT; i++) {
        if (acs712_read_plug(i, &cur) == ESP_OK) {
            total += cur;
            ok++;
        }
    }
    *out = (ok > 0) ? total : 0.0f;
    if (ok == 0) {
        ESP_LOGW(TAG, "ACS712 read failed, current=0");
    }
    return ESP_OK;
}

esp_err_t electric_service_get_voltage_v(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    esp_err_t err = pzem_read_all(&s_pzem);
    if (err == ESP_OK && s_pzem.valid) {
        *out = s_pzem.voltage_v;
    } else {
        *out = 127.0f;
        ESP_LOGW(TAG, "PZEM read failed, voltage=127 default");
    }
    return ESP_OK;
}

esp_err_t electric_service_is_overload(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    float cur = 0.0f;
    electric_service_get_current_a(&cur);
    if (s_fsm_out) {
        float limit = s_fsm_out->per_plug_current_limit_a;
        *out = (cur > limit);
    } else {
        *out = false;
    }
    return ESP_OK;
}

esp_err_t electric_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    electric_fsm_force_safe_off(&s_fsm);
    return ESP_OK;
}

