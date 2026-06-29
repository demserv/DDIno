// @requirement RNF-ELECTRICAL-001 Circuit breaker por barramento com fail count
// @requirement RF-ENERGY-008 Proteção de sobrecorrente total via circuit breaker
#include "circuit_breaker.h"

#include <string.h>

#include "esp_timer.h"

static circuit_breaker_t s_breakers[CB_COUNT];

static const uint32_t DEFAULT_FAILURE_THRESHOLD = 5;
static const uint32_t DEFAULT_SUCCESS_THRESHOLD = 3;
static const uint32_t DEFAULT_HALF_OPEN_TIMEOUT_MS = 30000;
static const uint32_t DEFAULT_RECOVER_TIMEOUT_MS = 60000;

void circuit_breaker_init(void)
{
    memset(s_breakers, 0, sizeof(s_breakers));
    for (int i = 0; i < CB_COUNT; i++) {
        s_breakers[i].id = (cb_bus_id_t)i;
        s_breakers[i].state = CB_STATE_CLOSED;
        s_breakers[i].failure_threshold = DEFAULT_FAILURE_THRESHOLD;
        s_breakers[i].success_threshold = DEFAULT_SUCCESS_THRESHOLD;
        s_breakers[i].half_open_timeout_ms = DEFAULT_HALF_OPEN_TIMEOUT_MS;
        s_breakers[i].recover_timeout_ms = DEFAULT_RECOVER_TIMEOUT_MS;
    }
}

void circuit_breaker_record_success(cb_bus_id_t id)
{
    if (id >= CB_COUNT) return;
    circuit_breaker_t *cb = &s_breakers[id];
    cb->success_count++;
    cb->failure_count = 0;
    if (cb->state == CB_STATE_HALF_OPEN && cb->success_count >= cb->success_threshold) {
        cb->state = CB_STATE_CLOSED;
        cb->success_count = 0;
    }
}

void circuit_breaker_record_failure(cb_bus_id_t id)
{
    if (id >= CB_COUNT) return;
    circuit_breaker_t *cb = &s_breakers[id];
    cb->failure_count++;
    cb->last_failure_ms = esp_timer_get_time() / 1000ULL;
    if (cb->state == CB_STATE_CLOSED && cb->failure_count >= cb->failure_threshold) {
        cb->state = CB_STATE_OPEN;
        cb->opened_at_ms = cb->last_failure_ms;
    } else if (cb->state == CB_STATE_HALF_OPEN) {
        cb->state = CB_STATE_OPEN;
        cb->opened_at_ms = cb->last_failure_ms;
    }
}

bool circuit_breaker_is_available(cb_bus_id_t id)
{
    if (id >= CB_COUNT) return false;
    circuit_breaker_t *cb = &s_breakers[id];
    if (cb->state == CB_STATE_CLOSED) return true;
    if (cb->state == CB_STATE_HALF_OPEN) return true;
    return false;
}

cb_state_t circuit_breaker_get_state(cb_bus_id_t id)
{
    if (id >= CB_COUNT) return CB_STATE_OPEN;
    return s_breakers[id].state;
}

void circuit_breaker_reset(cb_bus_id_t id)
{
    if (id >= CB_COUNT) return;
    circuit_breaker_t *cb = &s_breakers[id];
    cb->state = CB_STATE_CLOSED;
    cb->failure_count = 0;
    cb->success_count = 0;
    cb->opened_at_ms = 0;
}

void circuit_breaker_update(void)
{
    uint64_t now_ms = esp_timer_get_time() / 1000ULL;
    for (int i = 0; i < CB_COUNT; i++) {
        circuit_breaker_t *cb = &s_breakers[i];
        if (cb->state == CB_STATE_OPEN) {
            if ((now_ms - cb->opened_at_ms) >= cb->half_open_timeout_ms) {
                cb->state = CB_STATE_HALF_OPEN;
                cb->success_count = 0;
            }
        }
    }
}

uint32_t circuit_breaker_open_count(cb_bus_id_t id)
{
    if (id >= CB_COUNT) return 0;
    return s_breakers[id].failure_count;
}

