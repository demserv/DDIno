#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ATOMIC_RING_SIZE 256
#define ATOMIC_ENTRY_MAX_LEN 128

typedef struct {
    uint32_t seq;
    uint32_t timestamp_ms;
    char     data[ATOMIC_ENTRY_MAX_LEN];
} atomic_ring_entry_t;

typedef struct {
    atomic_ring_entry_t entries[ATOMIC_RING_SIZE];
    volatile uint32_t   head;
    volatile uint32_t   tail;
} atomic_ring_t;

esp_err_t storage_atomic_init(void);
esp_err_t storage_atomic_append(uint32_t timestamp_ms, const char *data);
uint32_t  storage_atomic_available(void);
esp_err_t storage_atomic_read(uint32_t start_seq, atomic_ring_entry_t *out, uint32_t count, uint32_t *read);
esp_err_t storage_atomic_peek_latest(atomic_ring_entry_t *out);
esp_err_t storage_atomic_flush(void);

#ifdef __cplusplus
}
#endif
