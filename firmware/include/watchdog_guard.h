#ifndef FIRMWARE_INCLUDE_WATCHDOG_GUARD_H
#define FIRMWARE_INCLUDE_WATCHDOG_GUARD_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    WATCHDOG_SEVERITY_CRITICAL = 0,
    WATCHDOG_SEVERITY_NON_CRITICAL
} watchdog_severity_t;

typedef struct {
    int task_id;
    uint32_t missed_beats;
    uint32_t missed_threshold;
    bool alarmed;
    bool recovered;
    watchdog_severity_t severity;
} watchdog_status_t;

esp_err_t watchdog_guard_init(void);
void watchdog_guard_heartbeat(int task_id);
uint32_t watchdog_guard_get_heartbeat(int task_id);
watchdog_status_t watchdog_guard_check(int task_id);
void watchdog_guard_set_threshold(int task_id, uint32_t missed_threshold);
void watchdog_guard_set_severity(int task_id, watchdog_severity_t severity);
uint32_t watchdog_guard_get_alive_count(void);

#endif
