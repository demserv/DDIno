#include "ui_lvgl_mutex.h"
#include "esp_err.h"

SemaphoreHandle_t g_lvgl_mutex = NULL;

esp_err_t ui_lvgl_mutex_init(void)
{
    g_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (g_lvgl_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool ui_lvgl_mutex_take(TickType_t timeout_ticks)
{
    if (g_lvgl_mutex == NULL) return false;
    return xSemaphoreTakeRecursive(g_lvgl_mutex, timeout_ticks) == pdTRUE;
}

bool ui_lvgl_mutex_give(void)
{
    if (g_lvgl_mutex == NULL) return false;
    return xSemaphoreGiveRecursive(g_lvgl_mutex) == pdTRUE;
}
