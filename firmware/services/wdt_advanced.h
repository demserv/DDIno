#ifndef FIRMWARE_SERVICES_WDT_ADVANCED_H
#define FIRMWARE_SERVICES_WDT_ADVANCED_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define WDT_TASK_MAIN_LOOP   0
#define WDT_TASK_UI          1
#define WDT_TASK_WEB         2
#define WDT_TASK_COUNT       3

esp_err_t wdt_advanced_init(void);
esp_err_t wdt_advanced_register(int task_id, uint32_t timeout_ms);
void wdt_advanced_reset(int task_id);
void wdt_advanced_suspend(int task_id);
void wdt_advanced_resume(int task_id);
bool wdt_advanced_is_enabled(int task_id);

#endif
