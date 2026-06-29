#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SemaphoreHandle_t g_lvgl_mutex;

esp_err_t ui_lvgl_mutex_init(void);
bool ui_lvgl_mutex_take(TickType_t timeout_ticks);
bool ui_lvgl_mutex_give(void);

#ifdef __cplusplus
}
#endif
