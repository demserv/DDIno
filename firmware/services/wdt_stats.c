// @requirement RF-WDT-004 Contador de resets WDT em janela de 24h
#include "wdt_stats.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "wdt_stats";
static const char *NS = "wdt_stats";

static uint32_t s_resets_24h;

static bool is_wdt_reset(esp_reset_reason_t rr)
{
    return (rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT ||
            rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT ||
            rr == ESP_RST_BROWNOUT);
}

esp_err_t wdt_stats_init(uint64_t now_s)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open falhou: %s", esp_err_to_name(err));
        s_resets_24h = is_wdt_reset(esp_reset_reason()) ? 1U : 0U;
        return err;
    }

    uint32_t count = 0;
    uint64_t window_start = 0;
    nvs_get_u32(h, "count", &count);
    nvs_get_u64(h, "win_s", &window_start);

    if (window_start == 0 || (now_s - window_start) >= 86400ULL) {
        count = 0;
        window_start = now_s;
    }

    if (is_wdt_reset(esp_reset_reason())) {
        count++;
        ESP_LOGW(TAG, "Reset anomalo detectado, count_24h=%lu", (unsigned long)count);
    }

    s_resets_24h = count;
    nvs_set_u32(h, "count", count);
    nvs_set_u64(h, "win_s", window_start);
    nvs_commit(h);
    nvs_close(h);
    return ESP_OK;
}

uint32_t wdt_stats_get_resets_24h(void)
{
    return s_resets_24h;
}
