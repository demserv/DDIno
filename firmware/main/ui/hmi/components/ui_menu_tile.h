#pragma once

#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *icon_label;
    lv_obj_t *text_label;
    lv_color_t saved_bg;
} ui_menu_tile_t;

typedef void (*ui_menu_tile_cb_t)(lv_event_t *e);

void ui_menu_tile_create(ui_menu_tile_t *tile, lv_obj_t *parent, int x, int y, const char *icon, const char *text);
void ui_menu_tile_set_callback(ui_menu_tile_t *tile, ui_menu_tile_cb_t cb, void *user_data);
void ui_menu_tile_set_focus(ui_menu_tile_t *tile, bool focused);
