/*
 * @requirement RF-STORAGE-001 RF-STORAGE-002 RF-STORAGE-003
 * @requirement RNF-STORAGE-006 RF-PERSIST-001
 * Facade único de armazenamento — encapsula NVS, SD e RAM fallback.
 */
#ifndef SERVICES_STORAGE_FACADE_H
#define SERVICES_STORAGE_FACADE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    STORAGE_CHAN_CONFIG_NVS = 0,
    STORAGE_CHAN_AUDIT_LOG,
    STORAGE_CHAN_ENERGY_CSV,
    STORAGE_CHAN_FEED_SNAPSHOT,
    STORAGE_CHAN_PARAMS_BLOB,
    STORAGE_CHAN_HISTORY,
    STORAGE_CHAN_COUNT
} storage_chan_t;

typedef struct {
    bool   nvs_ok;
    bool   sd_ok;
    bool   ram_fallback_active;
    size_t ram_fallback_bytes;
    size_t ram_fallback_dropped;
} storage_health_t;

esp_err_t storage_facade_init(void);
esp_err_t storage_facade_write(storage_chan_t ch, const void *buf, size_t len);
esp_err_t storage_facade_flush(storage_chan_t ch);
esp_err_t storage_facade_get_health(storage_health_t *out);
uint16_t storage_facade_audit_read_recent(char lines[][256], uint16_t max_lines);

#endif
