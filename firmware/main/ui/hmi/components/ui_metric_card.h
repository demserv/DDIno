// @requirement RF-ENERGY-001 Card de métrica com valor, unidade, ícone
#pragma once

#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *value_label;
    lv_obj_t *unit_label;
    lv_obj_t *label;
} ui_metric_card_t;

void ui_metric_card_create(ui_metric_card_t *card, lv_obj_t *parent, int x, int y, int w, int h, const char *title);
void ui_metric_card_set_value(ui_metric_card_t *card, const char *value, const char *unit);
void ui_metric_card_set_color(ui_metric_card_t *card, lv_color_t color);
