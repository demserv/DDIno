// @requirement RF-PLUG-006 Snapshot Feed Mode com checksum e TTL 30min
// @requirement RF-PLUG-006.1 Restauração pós-queda de energia
#ifndef FIRMWARE_SERVICES_FEED_SNAPSHOT_H
#define FIRMWARE_SERVICES_FEED_SNAPSHOT_H

#include "esp_err.h"
#include "fsm/feed_fsm.h"
#include <stdint.h>
#include <stdbool.h>

#define FEED_SNAPSHOT_NAMESPACE "feed_snap"
#define FEED_SNAPSHOT_KEY       "snapshot"
#define FEED_SNAPSHOT_MAX_AGE_S (30 * 60)

typedef struct {
    feed_state_t state;
    uint32_t duration_s;
    uint32_t cooldown_s;
    uint32_t pumps_off_mask;
    uint8_t  feed_count_1h;
    uint64_t state_started_rtc_s;
    uint64_t window_start_1h_rtc_s;
    bool     rtc_valid;
    uint64_t snapshot_time_s;
    uint32_t checksum;
} feed_snapshot_t;

esp_err_t feed_snapshot_save(const feed_fsm_t *fsm, uint64_t rtc_now_s, bool rtc_valid);
esp_err_t feed_snapshot_restore(feed_fsm_t *fsm, uint64_t rtc_now_s, uint64_t boot_now_ms, bool rtc_valid);
esp_err_t feed_snapshot_clear(void);
bool feed_snapshot_exists(void);

#endif
