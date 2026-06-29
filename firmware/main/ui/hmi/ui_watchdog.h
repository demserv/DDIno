#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_watchdog_timeout_cb_t)(void);

typedef struct {
    uint64_t        last_kick_ms;
    uint32_t        timeout_ms;
    bool            armed;
    ui_watchdog_timeout_cb_t on_timeout;
} ui_watchdog_t;

void ui_watchdog_init(ui_watchdog_t *wd, uint32_t timeout_ms, ui_watchdog_timeout_cb_t cb);
void ui_watchdog_kick(ui_watchdog_t *wd);
void ui_watchdog_arm(ui_watchdog_t *wd);
void ui_watchdog_disarm(ui_watchdog_t *wd);
bool ui_watchdog_check(ui_watchdog_t *wd, uint64_t now_ms);

#ifdef __cplusplus
}
#endif
