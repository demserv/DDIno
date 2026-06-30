// @requirement RF-UI-INPUT-001 Keypad AD 5 botoes via MCP3208
#ifndef DRIVERS_DRIVER_AD_KEYPAD_LVGL_H
#define DRIVERS_DRIVER_AD_KEYPAD_LVGL_H

#include "esp_err.h"
#include <stdbool.h>
#include "lvgl.h"

#define LV_KEY_HOME 0x80

esp_err_t driver_ad_keypad_lvgl_init(void);
bool ui_keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

#endif
