// @requirement RF-PLUG-003 Proteção de corrente por plugue
// @requirement RF-PLUG-014 Curto-circuito e sobrecarga extrema por plugue
// @requirement RNF-CALIB-001 Calibração assistida de sensores
#include "driver_acs712.h"
#include "driver_mcp3208.h"
#include "pin_map.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <string.h>
#include <math.h>

static const char *TAG = "acs712";

static acs712_chan_t s_acs712[ACS712_CHANNEL_COUNT];

static int plug_to_index(uint8_t plug_id)
{
    if (plug_id < 1 || plug_id > 10) return -1;
    return (int)(plug_id - 1);
}

static float compute_rms(int cs_gpio, uint8_t adc_channel, float offset_mv, float mv_per_a)
{
    float sum_sq = 0.0f;
    unsigned int valid = 0;
    for (unsigned int i = 0; i < ACS712_RMS_SAMPLES; i++) {
        uint16_t raw = 0;
        if (mcp3208_read_channel(cs_gpio, adc_channel, &raw) == ESP_OK) {
            float v = (float)raw * ACS712_ADC_REF_MV / 4095.0f;
            float diff = v - offset_mv;
            sum_sq += diff * diff;
            valid++;
        }
        esp_rom_delay_us(1000);
    }
    if (valid == 0) return 0.0f;
    float mean_sq = sum_sq / (float)valid;
    return sqrtf(mean_sq) / mv_per_a;
}

esp_err_t acs712_init(void)
{
    memset(s_acs712, 0, sizeof(s_acs712));

    for (int i = 0; i < 8; i++) {
        s_acs712[i].cs_gpio = PIN_ADC1_CS_GPIO;
        s_acs712[i].adc_channel = (uint8_t)i;
        s_acs712[i].zero_offset_mv = ACS712_ZERO_OFFSET_MV;
        s_acs712[i].mv_per_a = ACS712_MV_PER_A;
        s_acs712[i].calibrated = false;
        s_acs712[i].valid = false;
    }

    s_acs712[8].cs_gpio = PIN_ADC2_CS_GPIO;
    s_acs712[8].adc_channel = MCP3208_CH_P09_ACS;
    s_acs712[8].zero_offset_mv = ACS712_ZERO_OFFSET_MV;
    s_acs712[8].mv_per_a = ACS712_MV_PER_A;
    s_acs712[8].calibrated = false;
    s_acs712[8].valid = false;

    s_acs712[9].cs_gpio = PIN_ADC2_CS_GPIO;
    s_acs712[9].adc_channel = MCP3208_CH_P10_ACS;
    s_acs712[9].zero_offset_mv = ACS712_ZERO_OFFSET_MV;
    s_acs712[9].mv_per_a = ACS712_MV_PER_A;
    s_acs712[9].calibrated = false;
    s_acs712[9].valid = false;

    ESP_LOGI(TAG, "ACS712 init: 10 canais configurados para leitura RMS");
    return ESP_OK;
}

esp_err_t acs712_read_plug(uint8_t plug_id, float *current_a_rms)
{
    if (current_a_rms == NULL) return ESP_ERR_INVALID_ARG;

    int idx = plug_to_index(plug_id);
    if (idx < 0) return ESP_ERR_INVALID_ARG;

    acs712_chan_t *ch = &s_acs712[idx];

    float rms = compute_rms(ch->cs_gpio, ch->adc_channel, ch->zero_offset_mv, ch->mv_per_a);
    ch->current_a_rms = rms;
    ch->valid = (rms >= 0.0f);

    if (!ch->calibrated) {
        ESP_LOGW(TAG, "P%02d: sem calibracao zero, operando em modo degradado", plug_id);
    }

    *current_a_rms = rms;
    return ch->valid ? ESP_OK : ESP_FAIL;
}

esp_err_t acs712_calibrate_zero(uint8_t plug_id)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return ESP_ERR_INVALID_ARG;

    float sum = 0.0f;
    unsigned int valid = 0;
    for (unsigned int i = 0; i < ACS712_RMS_SAMPLES; i++) {
        uint16_t raw = 0;
        if (mcp3208_read_channel(s_acs712[idx].cs_gpio, s_acs712[idx].adc_channel, &raw) == ESP_OK) {
            sum += (float)raw;
            valid++;
        }
        esp_rom_delay_us(1000);
    }
    if (valid == 0) return ESP_FAIL;

    float mean_raw = sum / (float)valid;
    s_acs712[idx].zero_offset_mv = mean_raw * ACS712_ADC_REF_MV / 4095.0f;
    s_acs712[idx].calibrated = true;
    s_acs712[idx].valid = true;

    ESP_LOGI(TAG, "Calibracao zero P%02d: offset=%.1fmV (%u amostras)", plug_id, s_acs712[idx].zero_offset_mv, valid);
    return ESP_OK;
}

void acs712_set_zero_offset(uint8_t plug_id, float offset_mv)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return;
    s_acs712[idx].zero_offset_mv = offset_mv;
    s_acs712[idx].calibrated = true;
}

float acs712_get_zero_offset(uint8_t plug_id)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return ACS712_ZERO_OFFSET_MV;
    return s_acs712[idx].zero_offset_mv;
}

void acs712_set_mv_per_a(uint8_t plug_id, float mv_per_a)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return;
    s_acs712[idx].mv_per_a = mv_per_a;
}

float acs712_get_mv_per_a(uint8_t plug_id)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return ACS712_MV_PER_A;
    return s_acs712[idx].mv_per_a;
}

bool acs712_is_calibrated(uint8_t plug_id)
{
    int idx = plug_to_index(plug_id);
    if (idx < 0) return false;
    return s_acs712[idx].calibrated;
}
