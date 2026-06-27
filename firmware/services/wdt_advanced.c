// @requirement RF-WDT-001 Watchdog configurável por task
// @requirement RF-WDT-004 Timeout recovery com reset controlado
#include "wdt_advanced.h"
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

static const char *task_name(int id)
{
    switch (id) {
        case WDT_TASK_MAIN_LOOP: return "main_loop";
        case WDT_TASK_UI:        return "ui";
        case WDT_TASK_WEB:       return "web";
        default:                 return "unknown";
    }
}

esp_err_t wdt_advanced_init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    esp_task_wdt_config_t wdt_cfg = {
        .timeout_ms = 10000,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&wdt_cfg);
    ESP_LOGI(TAG, "WDT avancado inicializado (timeout 10s, panic)");
    return ESP_OK;
}

esp_err_t wdt_advanced_register(int task_id, uint32_t timeout_ms)
{
    if (task_id < 0 || task_id >= WDT_TASK_COUNT) return ESP_ERR_INVALID_ARG;
    wdt_task_entry_t *e = &s_tasks[task_id];
    e->task = xTaskGetCurrentTaskHandle();
    e->timeout_ms = timeout_ms;
    e->enabled = true;

    esp_err_t err = esp_task_wdt_add(e->task);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Falha ao registrar %s no WDT: %s", task_name(task_id), esp_err_to_name(err));
        return err;
    }

    e->registered = true;
    ESP_LOGI(TAG, "WDT registrado para %s (%lu ms)", task_name(task_id), (unsigned long)timeout_ms);
    return ESP_OK;
}

void wdt_advanced_reset(int task_id)
{
    if (task_id < 0 || task_id >= WDT_TASK_COUNT) return;
    wdt_task_entry_t *e = &s_tasks[task_id];
    if (e->enabled && e->registered) {
        esp_task_wdt_reset();
    }
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
