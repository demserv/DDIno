#include "driver_acs712.h"
#include "driver_mcp3208.h"
#include "pin_map.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "acs712";

static acs712_chan_t s_acs712[ACS712_CHANNEL_COUNT];

static int plug_to_index(uint8_t plug_id)
{
    if (plug_id < 1 || plug_id > 10) return -1;
    return (int)(plug_id - 1);
}

esp_err_t acs712_init(void)
{
    memset(s_acs712, 0, sizeof(s_acs712));

    for (int i = 0; i < 8; i++) {
        s_acs712[i].cs_gpio = PIN_ADC1_CS_GPIO;
        s_acs712[i].adc_channel = (uint8_t)i;
        s_acs712[i].zero_offset_mv = ACS712_ZERO_OFFSET_MV;
        s_acs712[i].mv_per_a = ACS712_MV_PER_A;
        s_acs712[i].valid = false;
    }

    s_acs712[8].cs_gpio = PIN_ADC2_CS_GPIO;
    s_acs712[8].adc_channel = MCP3208_CH_P09_ACS;
    s_acs712[8].zero_offset_mv = ACS712_ZERO_OFFSET_MV;
    s_acs712[8].mv_per_a = ACS712_MV_PER_A;
    s_acs712[8].valid = false;

    s_acs712[9].cs_gpio = PIN_ADC2_CS_GPIO;
    s_acs712[9].adc_channel = MCP3208_CH_P10_ACS;
    s_acs712[9].zero_offset_mv = ACS712_ZERO_OFFSET_MV;
    s_acs712[9].mv_per_a = ACS712_MV_PER_A;
    s_acs712[9].valid = false;

    ESP_LOGI(TAG, "ACS712 init: 10 canais configurados");
    return ESP_OK;
}

esp_err_t acs712_read_plug(uint8_t plug_id, float *current_a)
{
    if (current_a == NULL) return ESP_ERR_INVALID_ARG;

    int idx = plug_to_index(plug_id);
    if (idx < 0) return ESP_ERR_INVALID_ARG;

    acs712_chan_t *ch = &s_acs712[idx];

    uint16_t adc_raw = 0;
    esp_err_t err = mcp3208_read_channel(ch->cs_gpio, ch->adc_channel, &adc_raw);
    if (err != ESP_OK) {
        ch->valid = false;
        *current_a = 0.0f;
        return err;
    }

    float voltage_mv = (float)adc_raw * ACS712_ADC_REF_MV / 4095.0f;
    *current_a = (voltage_mv - ch->zero_offset_mv) / ch->mv_per_a;
    ch->current_a = *current_a;
    ch->valid = true;
    return ESP_OK;
}

esp_err_t acs712_calibrate_zero(uint8_t plug_id)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return ESP_ERR_INVALID_ARG;

    uint16_t adc_raw = 0;
    esp_err_t err = mcp3208_read_channel(s_acs712[idx].cs_gpio, s_acs712[idx].adc_channel, &adc_raw);
    if (err != ESP_OK) return err;

    s_acs712[idx].zero_offset_mv = (float)adc_raw * ACS712_ADC_REF_MV / 4095.0f;
    s_acs712[idx].valid = true;

    ESP_LOGI(TAG, "Calibracao zero P%02d: offset=%.1fmV", plug_id, s_acs712[idx].zero_offset_mv);
    return ESP_OK;
}

void acs712_set_zero_offset(uint8_t plug_id, float offset_mv)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return;
    s_acs712[idx].zero_offset_mv = offset_mv;
}
