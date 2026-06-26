#include "ui_confirm_dialog.h"
#include "../ui_theme.h"

void ui_confirm_dialog_create(ui_confirm_dialog_t *dlg, lv_obj_t *parent, const char *message)
{
    dlg->active = false;

    dlg->root = lv_obj_create(parent);
    lv_obj_set_size(dlg->root, 300, 140);
    lv_obj_center(dlg->root);
    lv_obj_set_style_bg_color(dlg->root, UI_COLOR_PANEL_3, 0);
    lv_obj_set_style_radius(dlg->root, UI_RADIUS_TILE, 0);
    lv_obj_set_style_border_color(dlg->root, UI_COLOR_CYAN, 0);
    lv_obj_set_style_border_width(dlg->root, 2, 0);
    lv_obj_set_style_pad_all(dlg->root, 0, 0);
    lv_obj_clear_flag(dlg->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dlg->root, LV_OBJ_FLAG_HIDDEN);

    dlg->message_label = lv_label_create(dlg->root);
    lv_label_set_text(dlg->message_label, message);
    lv_obj_set_style_text_font(dlg->message_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(dlg->message_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(dlg->message_label, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_size(dlg->message_label, 260, 40);
    lv_label_set_long_mode(dlg->message_label, LV_LABEL_LONG_WRAP);

    dlg->confirm_btn = lv_btn_create(dlg->root);
    lv_obj_set_size(dlg->confirm_btn, 100, 28);
    lv_obj_align(dlg->confirm_btn, LV_ALIGN_BOTTOM_LEFT, 20, -15);
    lv_obj_set_style_bg_color(dlg->confirm_btn, UI_COLOR_CRITICAL, 0);
    lv_obj_set_style_radius(dlg->confirm_btn, UI_RADIUS_BUTTON, 0);

    dlg->confirm_label = lv_label_create(dlg->confirm_btn);
    lv_label_set_text(dlg->confirm_label, "Confirmar");
    lv_obj_center(dlg->confirm_label);

    dlg->cancel_btn = lv_btn_create(dlg->root);
    lv_obj_set_size(dlg->cancel_btn, 100, 28);
    lv_obj_align(dlg->cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lv_obj_set_style_bg_color(dlg->cancel_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(dlg->cancel_btn, UI_RADIUS_BUTTON, 0);

    dlg->cancel_label = lv_label_create(dlg->cancel_btn);
    lv_label_set_text(dlg->cancel_label, "Cancelar");
    lv_obj_center(dlg->cancel_label);
}

void ui_confirm_dialog_set_confirm_cb(ui_confirm_dialog_t *dlg, lv_event_cb_t cb)
{
    lv_obj_add_event_cb(dlg->confirm_btn, cb, LV_EVENT_CLICKED, dlg);
}

void ui_confirm_dialog_set_cancel_cb(ui_confirm_dialog_t *dlg, lv_event_cb_t cb)
{
    lv_obj_add_event_cb(dlg->cancel_btn, cb, LV_EVENT_CLICKED, dlg);
}

void ui_confirm_dialog_show(ui_confirm_dialog_t *dlg, bool show)
{
    dlg->active = show;
    if (show) {
        lv_obj_clear_flag(dlg->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(dlg->root);
    } else {
        lv_obj_add_flag(dlg->root, LV_OBJ_FLAG_HIDDEN);
    }
}
