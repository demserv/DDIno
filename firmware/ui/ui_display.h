#ifndef FIRMWARE_UI_DISPLAY_H
#define FIRMWARE_UI_DISPLAY_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#define LVGL_TICK_PERIOD_MS 5
#define DISP_BUF_SIZE       (480 * 10)

esp_err_t ui_display_init(void);
void ui_display_set_brightness(uint8_t percent);
uint8_t ui_display_get_brightness(void);
void ui_display_dim_on_inactivity(bool enable, uint32_t timeout_s);

#endif
