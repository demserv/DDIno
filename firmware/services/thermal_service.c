// @requirement RF-THERMAL-001..010 Gerenciamento térmico
// @requirement RF-DADOS-001 Fonte única de estado: a thermal_fsm autoritativa vive
// no laço de controle (app_main). Este serviço expõe os valores publicados pelo
// laço; não mantém FSM própria nem thresholds hardcoded.
#include "thermal_service.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "global_state.h"
#include "safety_controller.h"
#include "services/config_manager.h"

static const char *TAG = "thermal_svc";
static float s_setpoint = 25.0f;
static float s_current_c = 0.0f;
static bool s_sample_valid = false;
static bool s_heating = false;
static bool s_cooling = false;

esp_err_t thermal_service_init(void)
{
    const thermal_params_storage_t *tp = config_get_thermal();
    s_setpoint = tp ? tp->temp_normal_c : 25.0f;
    s_current_c = 0.0f;
    s_sample_valid = false;
    s_heating = false;
    s_cooling = false;
    ESP_LOGI(TAG, "Thermal service initialized (setpoint=%.1fC)", (double)s_setpoint);
    return ESP_OK;
}

void thermal_service_publish(float current_c, bool sample_valid, bool heating, bool cooling)
{
    s_current_c = current_c;
    s_sample_valid = sample_valid;
    s_heating = heating;
    s_cooling = cooling;
}

esp_err_t thermal_service_get_setpoint(float *out_c)
{
    if (!out_c) return ESP_ERR_INVALID_ARG;
    const thermal_params_storage_t *tp = config_get_thermal();
    if (tp) {
        s_setpoint = tp->temp_normal_c;
    }
    *out_c = s_setpoint;
    return ESP_OK;
}

esp_err_t thermal_service_set_setpoint(float c)
{
    s_setpoint = c;
    ESP_LOGI(TAG, "Setpoint set to %.1fC", c);
    return ESP_OK;
}

/* @requirement RF-THERMAL-001 Temperatura corrente da fonte autoritativa; sem
 * amostra válida retorna INVALID_STATE (não inventa 0 como leitura real). */
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
    *out = s_heating;
    return ESP_OK;
}

esp_err_t thermal_service_is_cooling(bool *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = s_cooling;
    return ESP_OK;
}

esp_err_t thermal_service_force_safe_off(const char *reason)
{
    global_state_t *gs = global_state_get_write_ptr();
    if (!gs) return ESP_ERR_INVALID_STATE;
    uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
    ESP_LOGW(TAG, "Force SAFE_OFF: %s", reason ? reason : "unspecified");
    return global_state_enter_safeoff(gs, SAFEOFF_REASON_THERMAL_CRITICAL,
                                      "ALM-026", reason ? reason : "thermal_service",
                                      now_s);
}
