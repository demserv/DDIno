// @requirement RF-UI-STATUS-001 Barra de status persistente (substituida por header grafico)
#ifndef FIRMWARE_UI_HEADER_H
#define FIRMWARE_UI_HEADER_H
#include "esp_err.h"
#include "lvgl.h"
esp_err_t ui_header_init(void);
void ui_header_update(void);
#endif
