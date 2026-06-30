// @requirement RF-ALERT-001 a RF-ALERT-006 Tela de alertas com ativos, histórico, ACK
// @requirement RF-UI-ALERTS-001 Filtro por categoria e histórico recente
#include "ui_screen_alerts.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include "../components/ui_alert_row.h"
#include "alert_model.h"
#include <stdio.h>

static ui_alert_row_t active_rows[UI_MAX_ALERTS];
static ui_alert_row_t history_rows[UI_MAX_ALERTS];
static lv_obj_t *badge_label = NULL;
static lv_obj_t *summary_label = NULL;
static lv_obj_t *history_title = NULL;
static lv_obj_t *filter_roller = NULL;
static ui_root_vm_t *s_alerts_vm = NULL;
static int s_category_filter = -1; /* -1 = todas */

static void ack_all_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_ACK_ALERT);
}

static void filter_changed_cb(lv_event_t *e)
{
    (void)e;
    if (!filter_roller) return;
    uint16_t sel = lv_roller_get_selected(filter_roller);
    s_category_filter = (int)sel - 1;
    if (s_alerts_vm) {
        ui_screen_alerts_update(s_alerts_vm);
    }
}

static bool alert_matches_filter(const ui_alert_vm_t *av)
{
    if (s_category_filter < 0) return true;
    return (int)av->category == s_category_filter;
}

void ui_screen_alerts_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    s_alerts_vm = vm;
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

    filter_roller = lv_roller_create(parent);
    lv_roller_set_options(filter_roller, "Todas\nProcesso\nSistema\nSeguranca", LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(filter_roller, 110);
    lv_obj_set_height(filter_roller, 40);
    lv_obj_set_pos(filter_roller, 300, 22);
    lv_obj_add_event_cb(filter_roller, filter_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

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
    s_alerts_vm = vm;
    char buf[64];

    uint8_t shown = 0;
    for (uint8_t i = 0; i < vm->alerts.active_count; i++) {
        if (alert_matches_filter(&vm->alerts.active_alerts[i])) {
            shown++;
        }
    }

    snprintf(buf, sizeof(buf), "%u Ativos", (unsigned)shown);
    lv_label_set_text(badge_label, buf);
    if (shown > 0) {
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

    uint8_t disp = 0;
    for (uint8_t i = 0; i < vm->alerts.active_count && disp < UI_MAX_ALERTS; i++) {
        if (!alert_matches_filter(&vm->alerts.active_alerts[i])) {
            continue;
        }
        ui_alert_row_update(&active_rows[disp], &vm->alerts.active_alerts[i]);
        lv_obj_clear_flag(active_rows[disp].root, LV_OBJ_FLAG_HIDDEN);
        disp++;
    }
    for (int i = disp; i < UI_MAX_ALERTS; i++) {
        lv_obj_add_flag(active_rows[i].root, LV_OBJ_FLAG_HIDDEN);
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
