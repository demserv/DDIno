// @requirement RF-UI-OVERLAY-001 Overlay crítico SAFE_OFF e EMERGENCY em lv_layer_top()
// @requirement RF-GLOBAL-004.1 Matriz de prioridade visual de overlays
#include "ui_critical_overlay.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "../ui_events.h"
#include "ui_overlay_cause.h"
#include "system_types.h"
#include <stdio.h>

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

/* @requirement RF-UI-OVERLAY-001 ACK inline: permite reconhecer os alertas
 * críticos ativos direto do overlay, sem navegar até a tela de Alertas. A
 * política de duplo-ACK é aplicada pelo handler do evento (alert_manager). */
static void ack_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_ACK_ALERT);
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

    ov->ack_btn = lv_btn_create(ov->root);
    lv_obj_set_size(ov->ack_btn, 100, 28);
    lv_obj_align(ov->ack_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(ov->ack_btn, UI_COLOR_HIGH, 0);
    lv_obj_set_style_radius(ov->ack_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(ov->ack_btn, ack_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *acklbl = lv_label_create(ov->ack_btn);
    lv_label_set_text(acklbl, "ACK");
    lv_obj_center(acklbl);

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

void ui_critical_overlay_set_cause(ui_critical_overlay_t *ov, safeoff_reason_t reason, bool emergency)
{
    const ui_overlay_cause_template_t *t = ui_overlay_cause_lookup(reason);
    if (!ov->title_label || !ov->message_label) return;

    lv_label_set_text(ov->title_label, t->title);

    char msg[384];
    snprintf(msg, sizeof(msg),
             "Ocorreu: %s\n\nImpacto: %s\n\nAcao: %s\n\n%s",
             t->occurred, t->impact, t->action, t->exit_hint);
    lv_label_set_text(ov->message_label, msg);
    lv_obj_set_size(ov->message_label, 420, emergency ? 140 : 120);
}
