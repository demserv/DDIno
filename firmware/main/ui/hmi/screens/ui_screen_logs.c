#include "ui_screen_logs.h"
#include "../ui_theme.h"

void ui_screen_logs_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Logs");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 210, 6);

    lv_obj_t *na = lv_label_create(parent);
    lv_label_set_text(na, "Logs indisponiveis");
    lv_obj_set_style_text_font(na, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(na, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(na, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *wait = lv_label_create(parent);
    lv_label_set_text(wait, "Aguardando integracao com log_manager");
    lv_obj_set_style_text_font(wait, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(wait, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(wait, LV_ALIGN_CENTER, 0, 10);
}

void ui_screen_logs_update(ui_root_vm_t *vm)
{
    (void)vm;
}
