#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "task_manager.h"
#include "services/wdt_advanced.h"
#include "services/watchdog_guard.h"
#include "drivers/ui_display.h"
#include "ui/hmi/ui_app.h"
#include "ui/hmi/ui_lvgl_mutex.h"
#include "lvgl.h"

static const char *TAG = "task_ui";

void task_ui_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "ui task iniciada");

    while (1) {
        wdt_advanced_reset(TASK_ID_UI);
        watchdog_guard_heartbeat(TASK_ID_UI);

        if (ui_lvgl_mutex_take(pdMS_TO_TICKS(100))) {
            ui_app_tick();
            lv_timer_handler();
            ui_lvgl_mutex_give();
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_UI));
    }
}
