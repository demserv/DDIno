/* @requirement RF-STORAGE-* Facade único */
#include "storage_facade.h"
#include "storage_sd.h"
#include "storage_manager.h"
#include "storage_atomic.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "storage_facade";

#define AUDIT_RAM_RING_LINES    128
#define AUDIT_RAM_RING_LINE_MAX 256

static char     s_audit_ring[AUDIT_RAM_RING_LINES][AUDIT_RAM_RING_LINE_MAX];
static uint16_t s_audit_head     = 0;
static uint16_t s_audit_count    = 0;
static uint32_t s_audit_dropped  = 0;
static bool     s_initialized    = false;

esp_err_t storage_facade_init(void)
{
    if (s_initialized) return ESP_OK;
    s_audit_head = 0;
    s_audit_count = 0;
    s_audit_dropped = 0;
    s_initialized = true;
    ESP_LOGI(TAG, "storage facade init");
    return ESP_OK;
}

static void audit_ring_push(const char *line)
{
    size_t len = strlen(line);
    if (len >= AUDIT_RAM_RING_LINE_MAX) len = AUDIT_RAM_RING_LINE_MAX - 1;
    memcpy(s_audit_ring[s_audit_head], line, len);
    s_audit_ring[s_audit_head][len] = '\0';
    s_audit_head = (s_audit_head + 1) % AUDIT_RAM_RING_LINES;
    if (s_audit_count < AUDIT_RAM_RING_LINES) {
        s_audit_count++;
    } else {
        s_audit_dropped++;
    }
}

esp_err_t storage_facade_write(storage_chan_t ch, const void *buf, size_t len)
{
    if (!buf || len == 0) return ESP_ERR_INVALID_ARG;
    switch (ch) {
        case STORAGE_CHAN_AUDIT_LOG: {
            if (storage_sd_is_mounted()) {
                return storage_sd_write_log(SD_LOG_TYPE_AUDIT, (const char *)buf);
            }
            audit_ring_push((const char *)buf);
            return ESP_OK;
        }
        case STORAGE_CHAN_ENERGY_CSV:
            if (!storage_sd_is_mounted()) return ESP_ERR_INVALID_STATE;
            return storage_sd_write_log(SD_LOG_TYPE_ENERGY, (const char *)buf);
        case STORAGE_CHAN_CONFIG_NVS:
        case STORAGE_CHAN_PARAMS_BLOB:
        case STORAGE_CHAN_FEED_SNAPSHOT:
        case STORAGE_CHAN_HISTORY:
            /* Estes canais não usam o facade ainda — clientes diretos. */
            return ESP_ERR_NOT_SUPPORTED;
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t storage_facade_flush(storage_chan_t ch)
{
    if (ch != STORAGE_CHAN_AUDIT_LOG) return ESP_ERR_NOT_SUPPORTED;
    if (!storage_sd_is_mounted()) return ESP_ERR_INVALID_STATE;
    uint16_t i = (s_audit_count < AUDIT_RAM_RING_LINES)
                 ? 0
                 : s_audit_head;
    uint16_t remaining = s_audit_count;
    while (remaining > 0) {
        storage_sd_write_log(SD_LOG_TYPE_AUDIT, s_audit_ring[i]);
        i = (i + 1) % AUDIT_RAM_RING_LINES;
        remaining--;
    }
    s_audit_count = 0;
    s_audit_head = 0;
    return ESP_OK;
}

esp_err_t storage_facade_get_health(storage_health_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    out->nvs_ok = true;  /* NVS sempre presente em ESP32 */
    out->sd_ok = storage_sd_is_mounted();
    out->ram_fallback_active = !out->sd_ok;
    out->ram_fallback_bytes  = (size_t)s_audit_count * AUDIT_RAM_RING_LINE_MAX;
    out->ram_fallback_dropped = s_audit_dropped;
    return ESP_OK;
}

uint16_t storage_facade_audit_read_recent(char lines[][256], uint16_t max_lines)
{
    if (!lines || max_lines == 0) return 0;

    uint16_t copied = 0;
    uint16_t count = s_audit_count;
    if (count == 0) return 0;

    uint16_t idx = (s_audit_head + AUDIT_RAM_RING_LINES - 1) % AUDIT_RAM_RING_LINES;
    for (uint16_t i = 0; i < count && copied < max_lines; i++) {
        strncpy(lines[copied], s_audit_ring[idx], 255);
        lines[copied][255] = '\0';
        copied++;
        idx = (idx + AUDIT_RAM_RING_LINES - 1) % AUDIT_RAM_RING_LINES;
    }
    return copied;
}
