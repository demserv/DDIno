// @requirement RF-ENERGY-003 Acumuladores de energia
// @requirement RF-ENERGY-010 Log periódico SD
#include "cdn_energy.h"
#include "storage_sd.h"
#include "hardware_config.h"
#include "esp_timer.h"
#include <string.h>

typedef struct {
    float energy_wh;
    uint64_t last_update_ms;
} cdn_entry_t;

static cdn_entry_t s_entries[CDN_PLUG_COUNT];

esp_err_t cdn_energy_init(void)
{
    memset(s_entries, 0, sizeof(s_entries));
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    for (int i = 0; i < CDN_PLUG_COUNT; i++)
    {
        s_entries[i].last_update_ms = now_ms;
    }
    return ESP_OK;
}

void cdn_energy_update(uint8_t plug_id, float current_a, float voltage_v, uint64_t delta_ms)
{
    if (plug_id == 0 || plug_id > CDN_PLUG_COUNT) return;
    uint8_t idx = plug_id - 1;
    float power_w = current_a * voltage_v;
    float inc = power_w * ((float)delta_ms / 3600000.0f);
    s_entries[idx].energy_wh += inc;
    if (s_entries[idx].energy_wh > 999999.9f) s_entries[idx].energy_wh = 999999.9f;
    if (s_entries[idx].energy_wh < 0.0f) s_entries[idx].energy_wh = 0.0f;
    s_entries[idx].last_update_ms = esp_timer_get_time() / USEC_PER_MSEC;
}

float cdn_energy_get_wh(uint8_t plug_id)
{
    if (plug_id == 0 || plug_id > CDN_PLUG_COUNT) return 0.0f;
    return s_entries[plug_id - 1].energy_wh;
}

float cdn_energy_get_total_wh(void)
{
    float total = 0.0f;
    for (int i = 0; i < CDN_PLUG_COUNT; i++)
    {
        total += s_entries[i].energy_wh;
    }
    return total;
}

void cdn_energy_reset(uint8_t plug_id)
{
    if (plug_id == 0 || plug_id > CDN_PLUG_COUNT) return;
    uint8_t idx = plug_id - 1;
    s_entries[idx].energy_wh = 0.0f;
    s_entries[idx].last_update_ms = esp_timer_get_time() / USEC_PER_MSEC;
}

void cdn_energy_reset_all(void)
{
    memset(s_entries, 0, sizeof(s_entries));
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    for (int i = 0; i < CDN_PLUG_COUNT; i++)
    {
        s_entries[i].last_update_ms = now_ms;
    }
}

void cdn_energy_log_to_sd(void)
{
    if (!storage_sd_is_mounted()) return;
    char line[256];
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    float total = cdn_energy_get_total_wh();
    snprintf(line, sizeof(line), "%llu,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
        (unsigned long long)(now_ms / USEC_PER_MSEC),
        (double)s_entries[0].energy_wh,
        (double)s_entries[1].energy_wh,
        (double)s_entries[2].energy_wh,
        (double)s_entries[3].energy_wh,
        (double)s_entries[4].energy_wh,
        (double)s_entries[5].energy_wh,
        (double)s_entries[6].energy_wh,
        (double)s_entries[7].energy_wh,
        (double)s_entries[8].energy_wh,
        (double)s_entries[9].energy_wh,
        (double)total);
    storage_sd_write_log(SD_LOG_TYPE_ENERGY, line);
}
