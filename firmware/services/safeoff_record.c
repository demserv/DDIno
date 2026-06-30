#include "safeoff_record.h"

#include <stdio.h>

#include "esp_log.h"

#include "storage_manager.h"
#include "string.h"

static const char *TAG = "safeoff_rec";
static safeoff_record_t s_rec;

esp_err_t safeoff_record_init(void)
{
    memset(&s_rec, 0, sizeof(s_rec));
    return safeoff_record_load();
}

esp_err_t safeoff_record_append(safeoff_reason_t reason, const char *source_alm, uint64_t now_s)
{
    if (s_rec.count >= SAFEOFF_RECORD_MAX_ENTRIES) {
        for (uint32_t i = 1; i < s_rec.count; i++) {
            s_rec.entries[i - 1] = s_rec.entries[i];
        }
        s_rec.count--;
    }

    safeoff_record_entry_t *e = &s_rec.entries[s_rec.count++];
    e->reason = reason;
    if (source_alm) {
        snprintf(e->source_alm, sizeof(e->source_alm), "%s", source_alm);
    } else {
        e->source_alm[0] = '\0';
    }
    snprintf(e->entered_at, sizeof(e->entered_at), "%llu", (unsigned long long)now_s);
    e->entered_epoch_s = now_s;
    e->duration_s = 0;
    e->resolved = false;
    e->resolved_at[0] = '\0';

    ESP_LOGI(TAG, "SAFE_OFF recorded reason=%d alm=%s", (int)reason, source_alm ? source_alm : "?");
    return safeoff_record_persist();
}

esp_err_t safeoff_record_resolve_latest(uint64_t now_s)
{
    for (int32_t i = (int32_t)s_rec.count - 1; i >= 0; i--) {
        if (!s_rec.entries[i].resolved) {
            safeoff_record_entry_t *e = &s_rec.entries[i];
            e->resolved = true;
            snprintf(e->resolved_at, sizeof(e->resolved_at), "%llu", (unsigned long long)now_s);
            if (e->entered_epoch_s > 0) {
                e->duration_s = now_s - e->entered_epoch_s;
            }
            ESP_LOGI(TAG, "SAFE_OFF resolved reason=%d duration=%llus", (int)e->reason, (unsigned long long)e->duration_s);
            return safeoff_record_persist();
        }
    }
    return ESP_ERR_NOT_FOUND;
}

const safeoff_record_t *safeoff_record_get(void)
{
    return &s_rec;
}

esp_err_t safeoff_record_persist(void)
{
    return storage_set_blob(STORAGE_NS_STATE, "safeoff_rec", &s_rec, sizeof(s_rec));
}

esp_err_t safeoff_record_load(void)
{
    size_t len = sizeof(s_rec);
    esp_err_t err = storage_get_blob(STORAGE_NS_STATE, "safeoff_rec", &s_rec, &len);
    if (err != ESP_OK) {
        memset(&s_rec, 0, sizeof(s_rec));
    }
    return err;
}

