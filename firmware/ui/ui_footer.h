// @requirement RF-UI-STATUS-001 Footer com status geral, uptime e contador de alertas
#ifndef FIRMWARE_UI_FOOTER_H
#define FIRMWARE_UI_FOOTER_H
#include "esp_err.h"
#include "lvgl.h"
esp_err_t ui_footer_init(void);
void ui_footer_update(void);
#endif
