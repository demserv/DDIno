/* @requirement RNF-USAB-002 */
#include "ui_inline_hint.h"
#include "../ui_theme.h"

static void hide_cb(lv_timer_t *t)
{
    lv_obj_t *root = (lv_obj_t *)t->user_data;
    if (root) lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t);
}

void ui_inline_hint_create(ui_inline_hint_t *hint, lv_obj_t *parent)
{
    hint->root = lv_obj_create(parent);
    lv_obj_set_size(hint->root, 464, 24);
    lv_obj_align(hint->root, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_color(hint->root, UI_COLOR_WARN, 0);
    lv_obj_set_style_bg_opa(hint->root, LV_OPA_20, 0);
    lv_obj_set_style_border_width(hint->root, 0, 0);
    lv_obj_set_style_pad_all(hint->root, UI_PAD_4, 0);
    lv_obj_set_style_radius(hint->root, UI_RADIUS_SMALL, 0);

    hint->label = lv_label_create(hint->root);
    lv_obj_set_style_text_font(hint->label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint->label, UI_COLOR_TEXT_MAIN, 0);
    lv_label_set_text(hint->label, "");
    lv_obj_center(hint->label);
    lv_obj_add_flag(hint->root, LV_OBJ_FLAG_HIDDEN);
}

void ui_inline_hint_show(ui_inline_hint_t *hint, const char *text, uint32_t ms)
{
    if (!hint || !hint->root) return;
    lv_label_set_text(hint->label, text ? text : "");
    lv_obj_clear_flag(hint->root, LV_OBJ_FLAG_HIDDEN);
    if (ms > 0) {
        lv_timer_t *t = lv_timer_create(hide_cb, ms, hint->root);
        lv_timer_set_repeat_count(t, 1);
    }
}

void ui_inline_hint_hide(ui_inline_hint_t *hint)
{
    if (hint && hint->root) lv_obj_add_flag(hint->root, LV_OBJ_FLAG_HIDDEN);
}
