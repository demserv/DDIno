/* @requirement RF-STORAGE-003 escrita atômica em ring */
#include "storage_atomic.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "stor_atomic";
static atomic_ring_t s_ring;
static bool s_initialized = false;
static uint32_t s_global_seq = 0;

esp_err_t storage_atomic_init(void)
{
    memset(&s_ring, 0, sizeof(s_ring));
    s_ring.head = 0;
    s_ring.tail = 0;
    s_global_seq = 0;
    s_initialized = true;
    ESP_LOGD(TAG, "Atomic ring buffer initialized (%d entries)", ATOMIC_RING_SIZE);
    return ESP_OK;
}

esp_err_t storage_atomic_append(uint32_t timestamp_ms, const char *data)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (!data) return ESP_ERR_INVALID_ARG;

    uint32_t next = (s_ring.head + 1) % ATOMIC_RING_SIZE;

    if (next == s_ring.tail) {
        s_ring.tail = (s_ring.tail + 1) % ATOMIC_RING_SIZE;
    }

    atomic_ring_entry_t *e = &s_ring.entries[s_ring.head];
    e->seq = s_global_seq++;
    e->timestamp_ms = timestamp_ms;
    strncpy(e->data, data, ATOMIC_ENTRY_MAX_LEN - 1);
    e->data[ATOMIC_ENTRY_MAX_LEN - 1] = '\0';

    s_ring.head = next;

    return ESP_OK;
}

uint32_t storage_atomic_available(void)
{
    if (!s_initialized) return 0;
    uint32_t avail;
    if (s_ring.head >= s_ring.tail) {
        avail = s_ring.head - s_ring.tail;
    } else {
        avail = ATOMIC_RING_SIZE - s_ring.tail + s_ring.head;
    }
    return avail;
}

esp_err_t storage_atomic_read(uint32_t start_seq, atomic_ring_entry_t *out, uint32_t count, uint32_t *read)
{
    if (!s_initialized || !out || !read) return ESP_ERR_INVALID_ARG;

    *read = 0;
    uint32_t idx = s_ring.tail;
    uint32_t found = 0;

    while (idx != s_ring.head && found < count) {
        if (s_ring.entries[idx].seq >= start_seq) {
            out[found++] = s_ring.entries[idx];
        }
        idx = (idx + 1) % ATOMIC_RING_SIZE;
    }

    *read = found;
    return ESP_OK;
}

esp_err_t storage_atomic_peek_latest(atomic_ring_entry_t *out)
{
    if (!s_initialized || !out) return ESP_ERR_INVALID_ARG;

    uint32_t avail = storage_atomic_available();
    if (avail == 0) return ESP_ERR_NOT_FOUND;

    uint32_t last_idx = (s_ring.head == 0) ? ATOMIC_RING_SIZE - 1 : s_ring.head - 1;
    *out = s_ring.entries[last_idx];
    return ESP_OK;
}

esp_err_t storage_atomic_flush(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    s_ring.head = 0;
    s_ring.tail = 0;
    return ESP_OK;
}

