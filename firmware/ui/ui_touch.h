#ifndef FIRMWARE_UI_TOUCH_H
#define FIRMWARE_UI_TOUCH_H

#include "esp_err.h"
#include <stdbool.h>
#include "lvgl.h"

esp_err_t ui_touch_init(void);
bool ui_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

#endif
