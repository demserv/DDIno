#include "wdt_advanced.h"
#include "hardware_config.h"
#include "task_manager.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "wdt_adv";

typedef struct {
    bool enabled;
    bool registered;
    TaskHandle_t task;
    uint32_t timeout_ms;
} wdt_task_entry_t;

static wdt_task_entry_t s_tasks[WDT_TASK_COUNT];

esp_err_t wdt_advanced_init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    esp_task_wdt_config_t wdt_cfg = {
        .timeout_ms = HW_WDT_ADV_TIMEOUT_MS,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&wdt_cfg);
    ESP_LOGI(TAG, "WDT avancado inicializado (timeout 10s, panic)");
    return ESP_OK;
}

esp_err_t wdt_advanced_reset(int task_id)
{
    if (task_id < 0 || task_id >= WDT_TASK_COUNT) return ESP_ERR_INVALID_ARG;

    if (!s_tasks[task_id].registered) {
        memset(&s_tasks[task_id], 0, sizeof(s_tasks[task_id]));
        s_tasks[task_id].task = xTaskGetCurrentTaskHandle();
        s_tasks[task_id].enabled = true;
        s_tasks[task_id].registered = true;
        esp_err_t err = esp_task_wdt_add(s_tasks[task_id].task);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "WDT add falhou task %d: %s", task_id, esp_err_to_name(err));
            return err;
        }
        const char *tn = task_manager_get_name(task_id);
        ESP_LOGI(TAG, "WDT registrado auto para task %d (%s)", task_id, tn ? tn : "?");
    }

    if (s_tasks[task_id].enabled) {
        return esp_task_wdt_reset();
    }
    return ESP_OK;
}

void wdt_advanced_suspend(int task_id)
{
    if (task_id < 0 || task_id >= WDT_TASK_COUNT) return;
    s_tasks[task_id].enabled = false;
}

void wdt_advanced_resume(int task_id)
{
    if (task_id < 0 || task_id >= WDT_TASK_COUNT) return;
    s_tasks[task_id].enabled = true;
}

bool wdt_advanced_is_enabled(int task_id)
{
    if (task_id < 0 || task_id >= WDT_TASK_COUNT) return false;
    return s_tasks[task_id].enabled;
}
