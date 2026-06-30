// @requirement RF-ALERT-001 a RF-ALERT-006 Componente de linha de alerta na UI
#include "ui_alert_row.h"
#include "../ui_events.h"
#include "../ui_theme.h"
#include <stdio.h>
#include <string.h>

static void row_ack_click_cb(lv_event_t *e)
{
    ui_alert_row_t *row = (ui_alert_row_t *)lv_event_get_user_data(e);
    if (!row || !row->ack_click_enabled || row->bound_alm_id <= 0) {
        return;
    }
    ui_events_ack_alert(row->bound_alm_id);
}

static lv_color_t severity_color(ui_alert_severity_t sev)
{
    switch (sev) {
        case UI_SEVERITY_INFO: return UI_COLOR_INFO;
        case UI_SEVERITY_WARNING: return UI_COLOR_WARN;
        case UI_SEVERITY_HIGH: return UI_COLOR_HIGH;
        case UI_SEVERITY_CRITICAL: return UI_COLOR_CRITICAL;
    }
    return UI_COLOR_TEXT_DIM;
}

void ui_alert_row_create(ui_alert_row_t *row, lv_obj_t *parent, int x, int y)
{
    row->root = lv_obj_create(parent);
    lv_obj_set_size(row->root, 460, 48);
    lv_obj_set_pos(row->root, x, y);
    lv_obj_set_style_bg_color(row->root, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(row->root, UI_RADIUS_CARD, 0);
    lv_obj_set_style_border_width(row->root, 0, 0);
    lv_obj_clear_flag(row->root, LV_OBJ_FLAG_SCROLLABLE);

    row->side_bar = lv_obj_create(row->root);
    lv_obj_set_size(row->side_bar, 5, 48);
    lv_obj_set_pos(row->side_bar, 0, 0);
    lv_obj_set_style_border_width(row->side_bar, 0, 0);
    lv_obj_set_style_radius(row->side_bar, 0, 0);
    lv_obj_clear_flag(row->side_bar, LV_OBJ_FLAG_SCROLLABLE);

    row->id_label = lv_label_create(row->root);
    lv_label_set_text(row->id_label, "ALM-000");
    lv_obj_set_style_text_font(row->id_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(row->id_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(row->id_label, 12, 5);

    row->severity_label = lv_label_create(row->root);
    lv_label_set_text(row->severity_label, "INFO");
    lv_obj_set_style_text_font(row->severity_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(row->severity_label, 82, 5);

    row->category_label = lv_label_create(row->root);
    lv_label_set_text(row->category_label, "Cat");
    lv_obj_set_style_text_font(row->category_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(row->category_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(row->category_label, 150, 5);

    row->timestamp_label = lv_label_create(row->root);
    lv_label_set_text(row->timestamp_label, "--:--");
    lv_obj_set_style_text_font(row->timestamp_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(row->timestamp_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(row->timestamp_label, 395, 5);

    row->message_label = lv_label_create(row->root);
    lv_label_set_text(row->message_label, "");
    lv_obj_set_style_text_font(row->message_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(row->message_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(row->message_label, 12, 22);

    row->ack_label = lv_label_create(row->root);
    lv_label_set_text(row->ack_label, "");
    lv_obj_set_style_text_font(row->ack_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(row->ack_label, 395, 22);

    row->silence_label = lv_label_create(row->root);
    lv_label_set_text(row->silence_label, "");
    lv_obj_set_style_text_font(row->silence_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(row->silence_label, 320, 5);

    row->action_label = lv_label_create(row->root);
    lv_label_set_text(row->action_label, "");
    lv_obj_set_style_text_font(row->action_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(row->action_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(row->action_label, 220, 22);

    row->meta_label = lv_label_create(row->root);
    lv_label_set_text(row->meta_label, "");
    lv_obj_set_style_text_font(row->meta_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(row->meta_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(row->meta_label, 12, 34);

    row->bound_alm_id = 0;
    row->ack_click_enabled = false;
    lv_obj_add_flag(row->root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row->root, row_ack_click_cb, LV_EVENT_CLICKED, row);
}

void ui_alert_row_set_ack_enabled(ui_alert_row_t *row, bool enabled, int16_t alm_id)
{
    if (!row) return;
    row->ack_click_enabled = enabled;
    row->bound_alm_id = enabled ? alm_id : 0;
}

void ui_alert_row_update(ui_alert_row_t *row, const ui_alert_vm_t *alert)
{
    lv_color_t sev_col = severity_color(alert->severity);
    lv_obj_set_style_bg_color(row->side_bar, sev_col, 0);

    lv_label_set_text(row->id_label, alert->id);

    lv_label_set_text(row->severity_label, alert->severity_text);
    lv_obj_set_style_text_color(row->severity_label, sev_col, 0);

    lv_label_set_text(row->category_label, alert->category_text);

    lv_label_set_text(row->timestamp_label, alert->timestamp);

    if (alert->silenced) {
        lv_label_set_text(row->silence_label, "MUTE");
        lv_obj_set_style_text_color(row->silence_label, UI_COLOR_WARN, 0);
        lv_obj_clear_flag(row->silence_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(row->silence_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (alert->acked) {
        lv_label_set_text(row->ack_label, "ACK");
        lv_obj_set_style_text_color(row->ack_label, UI_COLOR_OK, 0);
    } else if (alert->ack_required) {
        lv_label_set_text(row->ack_label, "ACK?");
        lv_obj_set_style_text_color(row->ack_label, UI_COLOR_CRITICAL, 0);
    } else {
        lv_label_set_text(row->ack_label, "PENDENTE");
        lv_obj_set_style_text_color(row->ack_label, UI_COLOR_WARN, 0);
    }

    lv_label_set_text(row->message_label, alert->message);

    if (alert->action_hint[0] != '\0') {
        lv_label_set_text(row->action_label, alert->action_hint);
        lv_obj_clear_flag(row->action_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(row->action_label, LV_OBJ_FLAG_HIDDEN);
    }

    {
        char meta[48];
        if (alert->related_plug_id > 0) {
            snprintf(meta, sizeof(meta), "P%02u val=%.1f",
                     (unsigned)alert->related_plug_id, (double)alert->value);
        } else if (alert->value != 0.0f) {
            snprintf(meta, sizeof(meta), "val=%.1f", (double)alert->value);
        } else {
            meta[0] = '\0';
        }
        if (meta[0] != '\0') {
            lv_label_set_text(row->meta_label, meta);
            lv_obj_clear_flag(row->meta_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(row->meta_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
