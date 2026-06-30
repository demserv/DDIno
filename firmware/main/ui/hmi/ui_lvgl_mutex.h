#ifndef HMI_UI_LVGL_MUTEX_H
#define HMI_UI_LVGL_MUTEX_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SemaphoreHandle_t g_lvgl_mutex;

esp_err_t ui_lvgl_mutex_init(void);
bool ui_lvgl_mutex_take(TickType_t timeout_ticks);
bool ui_lvgl_mutex_give(void);

/* Helper para chamadas seguras de lv_timer_handler. */
static inline void ui_lvgl_tick(TickType_t timeout_ticks)
{
    if (ui_lvgl_mutex_take(timeout_ticks)) {
        lv_timer_handler();
        ui_lvgl_mutex_give();
    }
}

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /* HMI_UI_LVGL_MUTEX_H */