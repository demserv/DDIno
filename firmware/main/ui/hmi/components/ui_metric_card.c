// @requirement RF-ENERGY-001 a RF-ENERGY-010 Card de métrica para valores de energia
#include "ui_metric_card.h"
#include "../ui_theme.h"
#include <stdio.h>

void ui_metric_card_create(ui_metric_card_t *card, lv_obj_t *parent, int x, int y, int w, int h, const char *title)
{
    card->root = lv_obj_create(parent);
    lv_obj_set_size(card->root, w, h);
    lv_obj_set_pos(card->root, x, y);
    lv_obj_set_style_bg_color(card->root, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(card->root, UI_RADIUS_CARD, 0);
    lv_obj_set_style_border_width(card->root, 0, 0);
    lv_obj_set_style_pad_all(card->root, 0, 0);
    lv_obj_clear_flag(card->root, LV_OBJ_FLAG_SCROLLABLE);

    card->label = lv_label_create(card->root);
    lv_label_set_text(card->label, title);
    lv_obj_set_style_text_font(card->label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(card->label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(card->label, LV_ALIGN_TOP_MID, 0, 2);

    card->value_label = lv_label_create(card->root);
    lv_label_set_text(card->value_label, "--");
    lv_obj_set_style_text_font(card->value_label, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(card->value_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(card->value_label, LV_ALIGN_CENTER, -8, 0);

    card->unit_label = lv_label_create(card->root);
    lv_label_set_text(card->unit_label, "");
    lv_obj_set_style_text_font(card->unit_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(card->unit_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(card->unit_label, card->value_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
}

void ui_metric_card_set_value(ui_metric_card_t *card, const char *value, const char *unit)
{
    lv_label_set_text(card->value_label, value);
    lv_label_set_text(card->unit_label, unit);
}

void ui_metric_card_set_color(ui_metric_card_t *card, lv_color_t color)
{
    lv_obj_set_style_text_color(card->value_label, color, 0);
}
