#ifndef FIRMWARE_UI_STATUS_BAR_H
#define FIRMWARE_UI_STATUS_BAR_H
#include "esp_err.h"
#include "lvgl.h"
esp_err_t ui_status_bar_init(void);
void ui_status_bar_update(void);
#endif
