// @requirement RF-UI-INPUT-001 Keypad AD 5 botoes via MCP3208
#ifndef DRIVERS_DRIVER_AD_KEYPAD_LVGL_H
#define DRIVERS_DRIVER_AD_KEYPAD_LVGL_H

#include "esp_err.h"
#include <stdbool.h>
#include "lvgl.h"

#define LV_KEY_HOME 0x80
#define LV_KEY_FEED 0x81
#define LV_KEY_MUTE 0x82

/* @requirement RF-UI-NAV-HOME-001 / RF-UI-SHORTCUT-001 Teclas especiais não passam
 * pelo grupo LVGL; são roteadas por callback para a camada de UI. */
typedef void (*keypad_special_cb_t)(int key);
void driver_ad_keypad_set_special_cb(keypad_special_cb_t cb);
void driver_ad_keypad_gesture_poll(void);

esp_err_t driver_ad_keypad_lvgl_init(void);
void ui_keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

#endif
