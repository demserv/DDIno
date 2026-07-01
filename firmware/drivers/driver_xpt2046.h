// @requirement RF-UI-INPUT-001 Touch XPT2046 SPI
#ifndef DRIVERS_DRIVER_XPT2046_H
#define DRIVERS_DRIVER_XPT2046_H

#include "esp_err.h"
#include <stdbool.h>
#include "lvgl.h"

esp_err_t driver_xpt2046_init(void);
/* @requirement RF-FLOW-BOOT-003 Status real de inicialização do touch p/ self-test. */
bool driver_xpt2046_is_ok(void);
void ui_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

#endif
