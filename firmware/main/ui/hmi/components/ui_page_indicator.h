// @requirement RF-UI-CAROUSEL-001 Indicador de página (pontos)
#pragma once

#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *dots[12];
    uint8_t count;
} ui_page_indicator_t;

void ui_page_indicator_create(ui_page_indicator_t *ind, lv_obj_t *parent, int x, int y, uint8_t count);
void ui_page_indicator_set_active(ui_page_indicator_t *ind, uint8_t index);
