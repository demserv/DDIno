#include "watchdog_guard.h"
#include "task_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "wdt_guard";

#define DEFAULT_MISSED_THRESHOLD 3

typedef struct {
    volatile uint32_t heartbeat_count;
    uint32_t last_seen_count;
    uint32_t missed_beats;
    uint32_t missed_threshold;
    bool alarmed;
    watchdog_severity_t severity;
} guard_entry_t;

static guard_entry_t s_guards[TASK_ID_COUNT];
static bool s_initialized = false;

esp_err_t watchdog_guard_init(void)
{
    memset(s_guards, 0, sizeof(s_guards));
    for (int i = 0; i < TASK_ID_COUNT; i++) {
        s_guards[i].missed_threshold = DEFAULT_MISSED_THRESHOLD;
        s_guards[i].severity = WATCHDOG_SEVERITY_NON_CRITICAL;
    }

    s_guards[TASK_ID_SAFETY_CORE].severity = WATCHDOG_SEVERITY_CRITICAL;
    s_guards[TASK_ID_SENSORS].severity = WATCHDOG_SEVERITY_CRITICAL;
    s_guards[TASK_ID_PLUG_CONTROL].severity = WATCHDOG_SEVERITY_CRITICAL;
    s_guards[TASK_ID_STORAGE].severity = WATCHDOG_SEVERITY_NON_CRITICAL;
    s_guards[TASK_ID_UI].severity = WATCHDOG_SEVERITY_NON_CRITICAL;
    s_guards[TASK_ID_WEB].severity = WATCHDOG_SEVERITY_NON_CRITICAL;
    s_guards[TASK_ID_DIAG].severity = WATCHDOG_SEVERITY_NON_CRITICAL;

    s_initialized = true;
    ESP_LOGI(TAG, "Watchdog guard inicializado (%d tasks)", TASK_ID_COUNT);
    return ESP_OK;
}

void watchdog_guard_heartbeat(int task_id)
{
    if (!s_initialized) return;
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return;
    s_guards[task_id].heartbeat_count++;
}

uint32_t watchdog_guard_get_heartbeat(int task_id)
{
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return 0;
    return s_guards[task_id].heartbeat_count;
}

watchdog_status_t watchdog_guard_check(int task_id)
{
    watchdog_status_t st;
    memset(&st, 0, sizeof(st));
    st.task_id = task_id;

    if (!s_initialized || task_id < 0 || task_id >= TASK_ID_COUNT) {
        return st;
    }

    guard_entry_t *g = &s_guards[task_id];

    if (g->heartbeat_count > g->last_seen_count) {
        g->last_seen_count = g->heartbeat_count;
        st.missed_beats = 0;
        st.recovered = g->alarmed;
        g->alarmed = false;
        return st;
    }

    g->last_seen_count = g->heartbeat_count;
    st.missed_beats = g->missed_beats + 1;
    g->missed_beats = st.missed_beats;
    st.severity = g->severity;

    if (st.missed_beats >= g->missed_threshold && !g->alarmed) {
        g->alarmed = true;
        st.alarmed = true;
        const char *name = task_manager_get_name(task_id);
        ESP_LOGW(TAG, "Task %s (%d) sem heartbeat apos %lu ciclos (severidade=%d)",
                 name ? name : "?", task_id,
                 (unsigned long)st.missed_beats, (int)g->severity);
    }

    return st;
}

void watchdog_guard_set_threshold(int task_id, uint32_t missed_threshold)
{
    if (!s_initialized) return;
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return;
    s_guards[task_id].missed_threshold = missed_threshold;
}

void watchdog_guard_set_severity(int task_id, watchdog_severity_t severity)
{
    if (!s_initialized) return;
    if (task_id < 0 || task_id >= TASK_ID_COUNT) return;
    s_guards[task_id].severity = severity;
}

uint32_t watchdog_guard_get_alive_count(void)
{
    uint32_t alive = 0;
    for (int i = 0; i < TASK_ID_COUNT; i++) {
        if (s_guards[i].heartbeat_count > 0) alive++;
    }
    return alive;
}
