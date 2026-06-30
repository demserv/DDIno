// @requirement RNF-SECURITY-001 Rate limiting geral da API REST (req/min por IP).
// Login usa max_login_attempts em api_auth.c (separado deste módulo).
#include "api_rate_limit.h"
#include "hardware_config.h"
#include <string.h>
#include "esp_timer.h"

typedef struct {
    uint32_t ip;
    uint32_t count;
    uint64_t window_start_ms;
} rate_limit_entry_t;

static rate_limit_entry_t s_entries[RATE_LIMIT_TRACKED_IPS];
static const uint64_t s_window_ms = RATE_LIMIT_WINDOW_S * MS_PER_SEC;

static int find_entry(uint32_t ip)
{
    for (int i = 0; i < RATE_LIMIT_TRACKED_IPS; i++) {
        if (s_entries[i].ip == ip && s_entries[i].count > 0) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void)
{
    for (int i = 0; i < RATE_LIMIT_TRACKED_IPS; i++) {
        if (s_entries[i].count == 0) {
            return i;
        }
    }
    return -1;
}

bool rate_limit_check(uint32_t ip_addr)
{
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    int idx = find_entry(ip_addr);

    if (idx < 0) {
        idx = find_free_slot();
        if (idx < 0) {
            return false;
        }
        s_entries[idx].ip = ip_addr;
        s_entries[idx].count = 1;
        s_entries[idx].window_start_ms = now_ms;
        return true;
    }

    rate_limit_entry_t *e = &s_entries[idx];

    if ((now_ms - e->window_start_ms) > s_window_ms) {
        e->count = 1;
        e->window_start_ms = now_ms;
        return true;
    }

    e->count++;
    return (e->count <= RATE_LIMIT_MAX_REQ);
}

void rate_limit_reset(uint32_t ip_addr)
{
    int idx = find_entry(ip_addr);
    if (idx >= 0) {
        memset(&s_entries[idx], 0, sizeof(rate_limit_entry_t));
    }
}
