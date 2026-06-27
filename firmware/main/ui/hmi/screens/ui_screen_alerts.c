// @requirement RF-ALERT-001 a RF-ALERT-006 Tela de alertas com ativos, histórico, ACK
#include "ui_screen_alerts.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include "../components/ui_alert_row.h"
#include <stdio.h>

static ui_alert_row_t active_rows[UI_MAX_ALERTS];
static ui_alert_row_t history_rows[UI_MAX_ALERTS];
static lv_obj_t *badge_label = NULL;
static lv_obj_t *summary_label = NULL;
static lv_obj_t *history_title = NULL;

static void ack_all_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_ACK_ALERT);
}

void ui_screen_alerts_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Alertas Ativos");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 14, 6);

    badge_label = lv_label_create(parent);
    lv_label_set_text(badge_label, "0 Ativos");
    lv_obj_set_style_text_font(badge_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(badge_label, 400, 5);

    summary_label = lv_label_create(parent);
    lv_label_set_text(summary_label, "");
    lv_obj_set_style_text_font(summary_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(summary_label, 14, 26);

    for (int i = 0; i < UI_MAX_ALERTS; i++) {
        ui_alert_row_create(&active_rows[i], parent, 0, 54 + i * 44);
    }

    history_title = lv_label_create(parent);
    lv_label_set_text(history_title, "Historico");
    lv_obj_set_style_text_font(history_title, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(history_title, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(history_title, 14, 188);

    for (int i = 0; i < UI_MAX_ALERTS; i++) {
        ui_alert_row_create(&history_rows[i], parent, 0, 214 + i * 44);
    }

    lv_obj_t *ack_btn = lv_btn_create(parent);
    lv_obj_set_size(ack_btn, 90, 24);
    lv_obj_set_pos(ack_btn, 375, 230);
    lv_obj_set_style_bg_color(ack_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(ack_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(ack_btn, ack_all_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ack_lbl = lv_label_create(ack_btn);
    lv_label_set_text(ack_lbl, "ACK Todos");
    lv_obj_center(ack_lbl);

    ui_screen_alerts_update(vm);
}

void ui_screen_alerts_update(ui_root_vm_t *vm)
{
    if (!vm) return;
    char buf[64];

    snprintf(buf, sizeof(buf), "%u Ativos", (unsigned)vm->alerts.active_count);
    lv_label_set_text(badge_label, buf);
    if (vm->alerts.active_count > 0) {
        lv_obj_set_style_text_color(badge_label, UI_COLOR_CRITICAL, 0);
    } else {
        lv_obj_set_style_text_color(badge_label, UI_COLOR_TEXT_MUTED, 0);
    }

    snprintf(buf, sizeof(buf), "C:%u A:%u M:%u I:%u",
             (unsigned)vm->alerts.critical_count,
             (unsigned)vm->alerts.high_count,
             (unsigned)vm->alerts.warning_count,
             (unsigned)vm->alerts.info_count);
    lv_label_set_text(summary_label, buf);

    for (int i = 0; i < UI_MAX_ALERTS; i++) {
        if (i < vm->alerts.active_count) {
            ui_alert_row_update(&active_rows[i], &vm->alerts.active_alerts[i]);
            lv_obj_clear_flag(active_rows[i].root, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(active_rows[i].root, LV_OBJ_FLAG_HIDDEN);
        }
    }

    for (int i = 0; i < UI_MAX_ALERTS; i++) {
        if (i < vm->alerts.history_count) {
            ui_alert_row_update(&history_rows[i], &vm->alerts.history_alerts[i]);
            lv_obj_clear_flag(history_rows[i].root, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(history_rows[i].root, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
