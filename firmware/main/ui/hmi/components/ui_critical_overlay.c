// @requirement RF-UI-OVERLAY-001 Overlay crítico SAFE_OFF e EMERGENCY em lv_layer_top()
// @requirement RF-GLOBAL-004.1 Matriz de prioridade visual de overlays
#include "ui_critical_overlay.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"

static void alerts_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_ALERTS);
}

static void diag_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_DIAGNOSTICS);
}

static void create_common_buttons(ui_critical_overlay_t *ov)
{
    ov->alerts_btn = lv_btn_create(ov->root);
    lv_obj_set_size(ov->alerts_btn, 100, 28);
    lv_obj_align(ov->alerts_btn, LV_ALIGN_BOTTOM_LEFT, 40, -20);
    lv_obj_set_style_bg_color(ov->alerts_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(ov->alerts_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(ov->alerts_btn, alerts_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *albl = lv_label_create(ov->alerts_btn);
    lv_label_set_text(albl, "Ver Alertas");
    lv_obj_center(albl);

    ov->diag_btn = lv_btn_create(ov->root);
    lv_obj_set_size(ov->diag_btn, 100, 28);
    lv_obj_align(ov->diag_btn, LV_ALIGN_BOTTOM_RIGHT, -40, -20);
    lv_obj_set_style_bg_color(ov->diag_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(ov->diag_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(ov->diag_btn, diag_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dlbl = lv_label_create(ov->diag_btn);
    lv_label_set_text(dlbl, "Diagnostico");
    lv_obj_center(dlbl);
}

void ui_critical_overlay_safeoff_create(ui_critical_overlay_t *ov, lv_obj_t *parent)
{
    ov->root = lv_obj_create(parent);
    lv_obj_set_size(ov->root, 480, 320);
    lv_obj_set_pos(ov->root, 0, 0);
    lv_obj_set_style_bg_color(ov->root, lv_color_hex(0x500000), 0);
    lv_obj_set_style_border_color(ov->root, UI_COLOR_CRITICAL, 0);
    lv_obj_set_style_border_width(ov->root, 3, 0);
    lv_obj_set_style_radius(ov->root, 0, 0);
    lv_obj_set_style_pad_all(ov->root, 10, 0);
    lv_obj_clear_flag(ov->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ov->root, LV_OBJ_FLAG_HIDDEN);

    ov->title_label = lv_label_create(ov->root);
    lv_label_set_text(ov->title_label, "SAFE_OFF ATIVO");
    lv_obj_set_style_text_font(ov->title_label, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(ov->title_label, UI_COLOR_WARN, 0);
    lv_obj_align(ov->title_label, LV_ALIGN_TOP_MID, 0, 30);

    ov->message_label = lv_label_create(ov->root);
    lv_label_set_text(ov->message_label, "Sistema protegido.\nCargas criticas desligadas conforme politica.\nVerifique alertas ativos.");
    lv_obj_set_style_text_font(ov->message_label, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(ov->message_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(ov->message_label, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_size(ov->message_label, 400, 80);
    lv_label_set_long_mode(ov->message_label, LV_LABEL_LONG_WRAP);

    create_common_buttons(ov);
    ov->visible = false;
}

void ui_critical_overlay_emergency_create(ui_critical_overlay_t *ov, lv_obj_t *parent)
{
    ov->root = lv_obj_create(parent);
    lv_obj_set_size(ov->root, 480, 320);
    lv_obj_set_pos(ov->root, 0, 0);
    lv_obj_set_style_bg_color(ov->root, lv_color_hex(0x780000), 0);
    lv_obj_set_style_border_color(ov->root, UI_COLOR_CRITICAL, 0);
    lv_obj_set_style_border_width(ov->root, 3, 0);
    lv_obj_set_style_radius(ov->root, 0, 0);
    lv_obj_set_style_pad_all(ov->root, 10, 0);
    lv_obj_clear_flag(ov->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ov->root, LV_OBJ_FLAG_HIDDEN);

    ov->title_label = lv_label_create(ov->root);
    lv_label_set_text(ov->title_label, "EMERGENCY");
    lv_obj_set_style_text_font(ov->title_label, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(ov->title_label, UI_COLOR_CRITICAL, 0);
    lv_obj_align(ov->title_label, LV_ALIGN_TOP_MID, 0, 30);

    ov->message_label = lv_label_create(ov->root);
    lv_label_set_text(ov->message_label, "Falha critica extrema.\nIntervencao necessaria.");
    lv_obj_set_style_text_font(ov->message_label, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(ov->message_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(ov->message_label, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_size(ov->message_label, 400, 60);
    lv_label_set_long_mode(ov->message_label, LV_LABEL_LONG_WRAP);

    create_common_buttons(ov);
    ov->visible = false;
}

void ui_critical_overlay_show(ui_critical_overlay_t *ov, bool show)
{
    ov->visible = show;
    if (show) {
        lv_obj_clear_flag(ov->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ov->root);
    } else {
        lv_obj_add_flag(ov->root, LV_OBJ_FLAG_HIDDEN);
    }
}
