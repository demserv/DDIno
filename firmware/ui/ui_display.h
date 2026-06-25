#ifndef FIRMWARE_UI_DISPLAY_H
#define FIRMWARE_UI_DISPLAY_H

#include "esp_err.h"

#define LVGL_TICK_PERIOD_MS 5
#define DISP_BUF_SIZE       (480 * 10)

esp_err_t ui_display_init(void);

#endif
