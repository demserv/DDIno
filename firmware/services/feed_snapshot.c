// @requirement RF-PLUG-006 Snapshot operacional do Feed Mode
// @requirement RF-PLUG-006.1 Pós-queda de energia durante Feed Mode
#include "services/feed_snapshot.h"

#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "feed_snap";

static uint32_t calc_checksum(const feed_snapshot_t *s)
{
    const uint8_t *bytes = (const uint8_t *)s;
    uint32_t sum = 0;
    size_t len = sizeof(feed_snapshot_t) - sizeof(s->checksum);
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 7) ^ bytes[i];
    }
    return sum;
}

esp_err_t feed_snapshot_save(const feed_fsm_t *fsm, uint64_t rtc_now_s, bool rtc_valid)
{
    if (!fsm) return ESP_ERR_INVALID_ARG;

    feed_snapshot_t snap;
    memset(&snap, 0, sizeof(snap));

    snap.state = fsm->state;
    snap.duration_s = fsm->duration_s;
    snap.cooldown_s = fsm->cooldown_s;
    snap.pumps_off_mask = fsm->pumps_off_mask;
    snap.feed_count_1h = fsm->feed_count_1h;
    snap.state_started_rtc_s = rtc_valid ? rtc_now_s : 0;
    snap.window_start_1h_rtc_s = rtc_valid ? rtc_now_s : 0;
    snap.rtc_valid = rtc_valid;
    snap.snapshot_time_s = rtc_now_s;
    snap.checksum = calc_checksum(&snap);

    nvs_handle_t h;
    esp_err_t err = nvs_open(FEED_SNAPSHOT_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(h, FEED_SNAPSHOT_KEY, &snap, sizeof(snap));
    if (err == ESP_OK) {
        nvs_commit(h);
    } else {
        ESP_LOGE(TAG, "nvs_set_blob failed: %s", esp_err_to_name(err));
    }
    nvs_close(h);

    ESP_LOGV(TAG, "snapshot saved state=%d rtc=%llu", (int)snap.state, (unsigned long long)rtc_now_s);
    return err;
}

esp_err_t feed_snapshot_restore(feed_fsm_t *fsm, uint64_t rtc_now_s, uint64_t boot_now_ms, bool rtc_valid)
{
    if (!fsm) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t err = nvs_open(FEED_SNAPSHOT_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    feed_snapshot_t snap;
    size_t stored = sizeof(snap);
    err = nvs_get_blob(h, FEED_SNAPSHOT_KEY, &snap, &stored);
    nvs_close(h);

    if (err != ESP_OK) return err;
    if (stored != sizeof(snap)) return ESP_ERR_NVS_NOT_FOUND;

    uint32_t expected_cs = calc_checksum(&snap);
    if (snap.checksum != expected_cs) {
        ESP_LOGW(TAG, "snapshot checksum mismatch, discarding");
        feed_snapshot_clear();
        return ESP_ERR_INVALID_CRC;
    }

    if (rtc_valid && snap.rtc_valid) {
        uint64_t age_s = rtc_now_s - snap.snapshot_time_s;
        if (age_s > FEED_SNAPSHOT_MAX_AGE_S) {
            ESP_LOGW(TAG, "snapshot expired (age=%llus), discarding", (unsigned long long)age_s);
            feed_snapshot_clear();
            return ESP_ERR_TIMEOUT;
        }
    }

    if (snap.state == FEED_STATE_IDLE) {
        return ESP_OK;
    }

    if (!rtc_valid || !snap.rtc_valid) {
        ESP_LOGW(TAG, "RTC not available, cannot restore feed state");
        feed_snapshot_clear();
        return ESP_ERR_INVALID_STATE;
    }

    uint64_t elapsed_s = rtc_now_s - snap.state_started_rtc_s;
    ESP_LOGI(TAG, "restoring feed state=%d elapsed=%llus", (int)snap.state, (unsigned long long)elapsed_s);

    if (snap.state == FEED_STATE_ACTIVE) {
        if (elapsed_s >= snap.duration_s) {
            uint32_t remaining = (uint32_t)(elapsed_s - snap.duration_s);
            if (remaining >= snap.cooldown_s) {
                feed_fsm_init(fsm, snap.duration_s, snap.cooldown_s);
                fsm->state = FEED_STATE_IDLE;
                fsm->pumps_off_mask = 0x0F;
            } else {
                feed_fsm_init(fsm, snap.duration_s, snap.cooldown_s - remaining);
                fsm->state = FEED_STATE_COOLDOWN;
                fsm->state_started_ms = boot_now_ms;
                fsm->pumps_off_mask = snap.pumps_off_mask;
            }
        } else {
            feed_fsm_init(fsm, (uint32_t)(snap.duration_s - elapsed_s), snap.cooldown_s);
            fsm->state = FEED_STATE_ACTIVE;
            fsm->state_started_ms = boot_now_ms;
            fsm->pumps_off_mask = snap.pumps_off_mask;
            ESP_LOGW(TAG, "feed resumed with %lus remaining", (unsigned long)(snap.duration_s - elapsed_s));
        }
    } else if (snap.state == FEED_STATE_COOLDOWN) {
        if (elapsed_s >= snap.cooldown_s) {
            feed_fsm_init(fsm, snap.duration_s, snap.cooldown_s);
            fsm->state = FEED_STATE_IDLE;
            fsm->pumps_off_mask = 0x0F;
        } else {
            feed_fsm_init(fsm, snap.duration_s, (uint32_t)(snap.cooldown_s - elapsed_s));
            fsm->state = FEED_STATE_COOLDOWN;
            fsm->state_started_ms = boot_now_ms;
            fsm->pumps_off_mask = snap.pumps_off_mask;
        }
    }

    if (snap.feed_count_1h > 0) {
        uint64_t window_elapsed_s = rtc_now_s - snap.window_start_1h_rtc_s;
        if (window_elapsed_s < (60 * 60)) {
            fsm->feed_count_1h = snap.feed_count_1h;
            fsm->window_start_1h_ms = boot_now_ms;
        } else {
            fsm->feed_count_1h = 0;
            fsm->window_start_1h_ms = 0;
        }
    }

    feed_snapshot_clear();
    return ESP_OK;
}

esp_err_t feed_snapshot_clear(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(FEED_SNAPSHOT_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_erase_key(h, FEED_SNAPSHOT_KEY);
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

bool feed_snapshot_exists(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(FEED_SNAPSHOT_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) return false;

    size_t stored = 0;
    err = nvs_get_blob(h, FEED_SNAPSHOT_KEY, NULL, &stored);
    nvs_close(h);
    return (err == ESP_OK && stored > 0);
}

