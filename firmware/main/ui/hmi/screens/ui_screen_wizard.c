// @requirement RF-UI-WIZARD-001..005 Wizard com 6 steps (welcome, password, thermal, ato, electric, review)
// @requirement RF-GLOBAL-006 Bloqueio de operação sem wizard concluído
#include "ui_screen_wizard.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"

static void wizard_back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
}

static void wizard_next_cb(lv_event_t *e)
{
    (void)e;
    /* TODO: Avancar wizard via ConfigManager */
}

void ui_screen_wizard_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Wizard Inicial");
    lv_obj_set_style_text_font(title, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *step = lv_label_create(parent);
    lv_label_set_text(step, "Etapa atual: nao integrada");
    lv_obj_set_style_text_font(step, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(step, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(step, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *info = lv_label_create(parent);
    lv_label_set_text(info, "Aguardando integracao com ConfigManager");
    lv_obj_set_style_text_font(info, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(info, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(info, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 100, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -15);
    lv_obj_set_style_bg_color(back_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(back_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(back_btn, wizard_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Voltar");
    lv_obj_center(back_lbl);

    lv_obj_t *next_btn = lv_btn_create(parent);
    lv_obj_set_size(next_btn, 100, 28);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lv_obj_set_style_bg_color(next_btn, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(next_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(next_btn, wizard_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "Avancar");
    lv_obj_center(next_lbl);
}

void ui_screen_wizard_update(ui_root_vm_t *vm)
{
    (void)vm;
}
