// @requirement RF-ELECTRIC-BIVOLT-001 Gerenciamento elétrico
// @requirement RF-DADOS-001 Fonte única de estado: a electric_fsm autoritativa e as
// leituras PZEM/ACS712 vivem no laço de controle (app_main). Este serviço expõe os
// valores publicados pelo laço; não mantém FSM própria nem refaz leituras de
// hardware fora do laço (evita concorrência no barramento e estado divergente).
#include "electric_service.h"

#include "esp_log.h"

static const char *TAG = "electric_svc";
static float s_power_w = 0.0f;
static float s_current_a = 0.0f;
static float s_voltage_v = 0.0f;
static bool s_overload = false;

esp_err_t electric_service_init(void)
{
    s_power_w = 0.0f;
    s_current_a = 0.0f;
    s_voltage_v = 0.0f;
    s_overload = false;
    ESP_LOGI(TAG, "Electric service initialized");
    return ESP_OK;
}

void electric_service_publish(float power_w, float current_a, float voltage_v, bool overload)
{
    s_power_w = power_w;
    s_current_a = current_a;
    s_voltage_v = voltage_v;
    s_overload = overload;
}

esp_err_t electric_service_get_power_w(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_power_w;
    return ESP_OK;
}

esp_err_t electric_service_get_current_a(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_current_a;
    return ESP_OK;
}

esp_err_t electric_service_get_voltage_v(float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_voltage_v;
    return ESP_OK;
}

esp_err_t electric_service_is_overload(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_overload;
    return ESP_OK;
}

esp_err_t electric_service_force_safe_off(const char *reason)
{
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return ESP_OK;
}
