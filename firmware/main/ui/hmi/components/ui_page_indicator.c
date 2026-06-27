// @requirement RF-UI-CAROUSEL-001 Indicador de página para navegação entre telas
#include "ui_page_indicator.h"
#include "../ui_theme.h"

void ui_page_indicator_create(ui_page_indicator_t *ind, lv_obj_t *parent, int x, int y, uint8_t count)
{
    ind->count = count;
    if (count > 12) count = 12;
    ind->root = lv_obj_create(parent);
    lv_obj_set_size(ind->root, count * 14, 12);
    lv_obj_set_pos(ind->root, x, y);
    lv_obj_set_style_bg_opa(ind->root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ind->root, 0, 0);
    lv_obj_clear_flag(ind->root, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t i = 0; i < count; i++) {
        ind->dots[i] = lv_label_create(ind->root);
        lv_label_set_text(ind->dots[i], ".");
        lv_obj_set_style_text_font(ind->dots[i], UI_FONT_NORMAL, 0);
        lv_obj_set_pos(ind->dots[i], i * 14, 0);
        lv_obj_set_style_text_color(ind->dots[i], UI_COLOR_TEXT_DIM, 0);
    }
}

void ui_page_indicator_set_active(ui_page_indicator_t *ind, uint8_t index)
{
    for (uint8_t i = 0; i < ind->count && i < 12; i++) {
        if (i == index) {
            lv_obj_set_style_text_color(ind->dots[i], UI_COLOR_TEXT_MAIN, 0);
        } else {
            lv_obj_set_style_text_color(ind->dots[i], UI_COLOR_TEXT_DIM, 0);
        }
    }
}
