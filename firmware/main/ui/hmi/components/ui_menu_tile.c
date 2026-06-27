// @requirement RF-UI-CAROUSEL-001 Tile de menu 140x72 com ícone e texto
#include "ui_menu_tile.h"
#include "../ui_theme.h"

void ui_menu_tile_create(ui_menu_tile_t *tile, lv_obj_t *parent, int x, int y, const char *icon, const char *text)
{
    tile->root = lv_obj_create(parent);
    lv_obj_set_size(tile->root, 140, 72);
    lv_obj_set_pos(tile->root, x, y);
    tile->saved_bg = UI_COLOR_PANEL;
    lv_obj_set_style_bg_color(tile->root, tile->saved_bg, 0);
    lv_obj_set_style_radius(tile->root, UI_RADIUS_TILE, 0);
    lv_obj_set_style_border_width(tile->root, 0, 0);
    lv_obj_set_style_pad_all(tile->root, 0, 0);
    lv_obj_clear_flag(tile->root, LV_OBJ_FLAG_SCROLLABLE);

    tile->icon_label = lv_label_create(tile->root);
    lv_label_set_text(tile->icon_label, icon);
    lv_obj_set_style_text_font(tile->icon_label, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(tile->icon_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(tile->icon_label, LV_ALIGN_TOP_MID, 0, 10);

    tile->text_label = lv_label_create(tile->root);
    lv_label_set_text(tile->text_label, text);
    lv_obj_set_style_text_font(tile->text_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(tile->text_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(tile->text_label, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void ui_menu_tile_set_callback(ui_menu_tile_t *tile, ui_menu_tile_cb_t cb, void *user_data)
{
    lv_obj_add_event_cb(tile->root, cb, LV_EVENT_CLICKED, user_data);
}

void ui_menu_tile_set_focus(ui_menu_tile_t *tile, bool focused)
{
    if (focused) {
        lv_obj_set_style_border_width(tile->root, 2, 0);
        lv_obj_set_style_border_color(tile->root, UI_COLOR_CYAN, 0);
        lv_obj_set_style_bg_color(tile->root, UI_COLOR_PANEL_2, 0);
    } else {
        lv_obj_set_style_border_width(tile->root, 0, 0);
        lv_obj_set_style_bg_color(tile->root, tile->saved_bg, 0);
    }
}
