// @requirement RF-ENERGY-003 Acumuladores de energia
// @requirement RF-ENERGY-010 Log periódico SD
// @requirement ALM-025 Acumulador mensal persistente
#include "cdn_energy.h"
#include "storage_sd.h"
#include "hardware_config.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <string.h>
#include <time.h>

#define CDN_NVS_NS   "cdn_energy"
#define CDN_NVS_KEY  "persist"

typedef struct {
    float energy_wh;
    uint64_t last_update_ms;
} cdn_entry_t;

typedef struct {
    float    wh_today;
    float    wh_month;
    float    wh_today_plug[CDN_PLUG_COUNT];
    int16_t  last_yday;
    int16_t  last_mon;
    int16_t  last_year;
    float    monthly_hist[6];
    uint8_t  hist_count;
} cdn_persist_t;

static cdn_entry_t s_entries[CDN_PLUG_COUNT];
static cdn_persist_t s_persist;

static esp_err_t persist_load(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(CDN_NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    size_t sz = sizeof(s_persist);
    err = nvs_get_blob(h, CDN_NVS_KEY, &s_persist, &sz);
    nvs_close(h);
    return err;
}

static esp_err_t persist_save(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(CDN_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, CDN_NVS_KEY, &s_persist, sizeof(s_persist));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t cdn_energy_init(void)
{
    memset(s_entries, 0, sizeof(s_entries));
    memset(&s_persist, 0, sizeof(s_persist));
    if (persist_load() != ESP_OK) {
        s_persist.last_yday = -1;
        s_persist.last_mon = -1;
        s_persist.last_year = -1;
    }
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    for (int i = 0; i < CDN_PLUG_COUNT; i++) {
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
    s_persist.wh_today_plug[idx] += inc;
    s_entries[idx].last_update_ms = esp_timer_get_time() / USEC_PER_MSEC;

    s_persist.wh_today += inc;
    s_persist.wh_month += inc;
}

float cdn_energy_get_wh_today_plug(uint8_t plug_id)
{
    if (plug_id == 0 || plug_id > CDN_PLUG_COUNT) return 0.0f;
    return s_persist.wh_today_plug[plug_id - 1];
}

float cdn_energy_get_wh(uint8_t plug_id)
{
    if (plug_id == 0 || plug_id > CDN_PLUG_COUNT) return 0.0f;
    return s_entries[plug_id - 1].energy_wh;
}

float cdn_energy_get_total_wh(void)
{
    float total = 0.0f;
    for (int i = 0; i < CDN_PLUG_COUNT; i++) {
        total += s_entries[i].energy_wh;
    }
    return total;
}

float cdn_energy_get_wh_today(void)
{
    return s_persist.wh_today;
}

float cdn_energy_get_wh_month(void)
{
    return s_persist.wh_month;
}

void cdn_energy_get_monthly_history(float out_kwh[6], uint8_t *count)
{
    if (!out_kwh || !count) return;
    out_kwh[0] = s_persist.wh_month / 1000.0f;
    uint8_t n = 1;
    for (uint8_t i = 0; i < s_persist.hist_count && n < 6; i++) {
        out_kwh[n++] = s_persist.monthly_hist[i] / 1000.0f;
    }
    *count = n;
}

void cdn_energy_tick(int year, int mon, int mday)
{
    if (year <= 0) return;

    if (s_persist.last_yday < 0) {
        s_persist.last_yday = (int16_t)mday;
        s_persist.last_mon = (int16_t)mon;
        s_persist.last_year = (int16_t)year;
        persist_save();
        return;
    }

    if (mday != s_persist.last_yday || mon != s_persist.last_mon || year != s_persist.last_year) {
        if (mon != s_persist.last_mon || year != s_persist.last_year) {
            for (int i = 5; i > 0; i--) {
                s_persist.monthly_hist[i] = s_persist.monthly_hist[i - 1];
            }
            s_persist.monthly_hist[0] = s_persist.wh_month;
            if (s_persist.hist_count < 6) s_persist.hist_count++;
            s_persist.wh_month = 0.0f;
        }
        if (mday != s_persist.last_yday) {
            s_persist.wh_today = 0.0f;
            memset(s_persist.wh_today_plug, 0, sizeof(s_persist.wh_today_plug));
        }
        s_persist.last_yday = (int16_t)mday;
        s_persist.last_mon = (int16_t)mon;
        s_persist.last_year = (int16_t)year;
        persist_save();
    }
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
    for (int i = 0; i < CDN_PLUG_COUNT; i++) {
        s_entries[i].last_update_ms = now_ms;
    }
}

void cdn_energy_log_to_sd(void)
{
    if (!storage_sd_is_mounted()) return;
    char line[256];
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    float total = cdn_energy_get_total_wh();
    snprintf(line, sizeof(line), "%llu,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
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
        (double)total,
        (double)s_persist.wh_today,
        (double)s_persist.wh_month);
    storage_sd_write_log(SD_LOG_TYPE_ENERGY, line);
}
