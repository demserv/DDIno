// @requirement RF-GLOBAL-003 Badge de estado global colorido
#pragma once

#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *label;
    lv_color_t bg_color;
} ui_status_badge_t;

void ui_status_badge_create(ui_status_badge_t *badge, lv_obj_t *parent, int x, int y, int w, int h);
void ui_status_badge_set_text(ui_status_badge_t *badge, const char *text);
void ui_status_badge_set_color(ui_status_badge_t *badge, lv_color_t color);
