// @requirement RF-UI-INPUT-001 Keypad AD 5 botoes via MCP3208
#ifndef FIRMWARE_UI_KEYPAD_H
#define FIRMWARE_UI_KEYPAD_H

#include "esp_err.h"
#include <stdbool.h>
#include "lvgl.h"

#define LV_KEY_HOME 0x80

esp_err_t ui_keypad_init(void);
bool ui_keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

#endif
