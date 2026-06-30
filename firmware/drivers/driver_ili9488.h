// @requirement RF-UI-DISPLAY-001 Brilho configurável
#ifndef DRIVERS_DRIVER_ILI9488_H
#define DRIVERS_DRIVER_ILI9488_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#define LVGL_TICK_PERIOD_MS 5
#define DISP_BUF_SIZE       (480 * 10)

esp_err_t driver_ili9488_init(void);
void ui_display_set_brightness(uint8_t percent);
uint8_t ui_display_get_brightness(void);
void ui_display_dim_on_inactivity(bool enable, uint32_t timeout_s);
void ui_display_lvgl_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
esp_err_t ui_display_set_backlight(bool enabled);

/* @requirement RF-DISP-BACKLIGHT-001 PWM via LEDC */
esp_err_t driver_ili9488_backlight_init(int gpio_num);
esp_err_t driver_ili9488_backlight_set(uint8_t percent);

#endif
