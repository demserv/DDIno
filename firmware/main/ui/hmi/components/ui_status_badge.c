#include "ui_status_badge.h"
#include "../ui_theme.h"

void ui_status_badge_create(ui_status_badge_t *badge, lv_obj_t *parent, int x, int y, int w, int h)
{
    badge->root = lv_obj_create(parent);
    lv_obj_set_size(badge->root, w, h);
    lv_obj_set_pos(badge->root, x, y);
    lv_obj_set_style_radius(badge->root, UI_RADIUS_SMALL, 0);
    lv_obj_set_style_border_width(badge->root, 0, 0);
    lv_obj_clear_flag(badge->root, LV_OBJ_FLAG_SCROLLABLE);

    badge->label = lv_label_create(badge->root);
    lv_label_set_text(badge->label, "");
    lv_obj_center(badge->label);
    lv_obj_set_style_text_font(badge->label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(badge->label, lv_color_hex(0xFFFFFF), 0);
}

void ui_status_badge_set_text(ui_status_badge_t *badge, const char *text)
{
    lv_label_set_text(badge->label, text);
}

void ui_status_badge_set_color(ui_status_badge_t *badge, lv_color_t color)
{
    badge->bg_color = color;
    lv_obj_set_style_bg_color(badge->root, color, 0);
}
