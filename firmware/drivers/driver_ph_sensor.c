/* @requirement RF-SENSOR-PH-001 Sensor de pH analogico (modulo AliExpress)
 * GPIO livre documentado em pin_map.h — decisao normativa 2026-06-30 (item 8). */
#include "driver_ph_sensor.h"
#include "hardware_config.h"
#include "pin_map.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ph_sensor";

static adc_oneshot_unit_handle_t s_adc = NULL;
static bool s_init = false;
static float s_last_ph = 0.0f;
static bool s_last_valid = false;

esp_err_t ph_sensor_init(void)
{
    if (s_init) return ESP_OK;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc);
    if (err != ESP_OK) return err;

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    err = adc_oneshot_config_channel(s_adc, (adc_channel_t)PIN_PH_ADC_CHANNEL, &chan_cfg);
    if (err != ESP_OK) {
        adc_oneshot_del_unit(s_adc);
        s_adc = NULL;
        return err;
    }

    gpio_reset_pin(PIN_PH_ADC_GPIO);
    s_init = true;
    ESP_LOGI(TAG, "pH sensor init GPIO%d (ADC1 CH%d)", PIN_PH_ADC_GPIO, PIN_PH_ADC_CHANNEL);
    return ESP_OK;
}

static float adc_to_ph(int raw)
{
    float mv = ((float)raw / (float)HW_ADC_MAX_COUNT) * (float)HW_ADC_VREF_MV;
    /* Linear 0..3300 mV → pH 0..14 (calibracao fina via wizard futuro). */
    float ph = (mv / (float)HW_ADC_VREF_MV) * HW_PH_SENSOR_SPAN;
    if (ph < 0.0f) ph = 0.0f;
    if (ph > HW_PH_SENSOR_SPAN) ph = HW_PH_SENSOR_SPAN;
    return ph;
}

esp_err_t ph_sensor_read(float *ph_out, bool *valid_out)
{
    if (!s_init || !ph_out || !valid_out) return ESP_ERR_INVALID_STATE;

    int raw = 0;
    esp_err_t err = adc_oneshot_read(s_adc, (adc_channel_t)PIN_PH_ADC_CHANNEL, &raw);
    if (err != ESP_OK) {
        *valid_out = false;
        return err;
    }

    s_last_ph = adc_to_ph(raw);
    s_last_valid = true;
    *ph_out = s_last_ph;
    *valid_out = true;
    return ESP_OK;
}

bool ph_sensor_get_last(float *ph_out)
{
    if (!s_last_valid) return false;
    if (ph_out) *ph_out = s_last_ph;
    return true;
}

bool ph_sensor_is_ok(void)
{
    return s_init && s_last_valid;
}
