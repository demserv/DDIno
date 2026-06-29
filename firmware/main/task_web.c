#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "services/watchdog_guard.h"
#include "services/wdt_advanced.h"
#include "services/web_ctl.h"
#include "services/wifi_ctl.h"
#include "task_manager.h"
#include "web/api_rest.h"

static const char *TAG = "task_web";

void task_web_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "web task iniciada");

    wifi_ctl_init();
    web_ctl_init();

    esp_err_t api_err = api_rest_init();
    if (api_err != ESP_OK) {
        ESP_LOGW(TAG, "API REST nao iniciou: %s", esp_err_to_name(api_err));
    }

    while (1) {
        wdt_advanced_reset(TASK_ID_WEB);
        watchdog_guard_heartbeat(TASK_ID_WEB);

        web_ctl_tick();

        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_WEB));
    }
}

