// @requirement RF-UI-STATUS-001 Topbar com WiFi, datetime, botão FEED
#include "ui_topbar.h"
#include "../ui_theme.h"
#include "../ui_events.h"

static void feed_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_FEED_MODE);
}

void ui_topbar_create(ui_topbar_t *bar, lv_obj_t *parent)
{
    bar->root = lv_obj_create(parent);
    lv_obj_set_size(bar->root, 480, 31);
    lv_obj_set_pos(bar->root, 0, 0);
    lv_obj_set_style_bg_color(bar->root, UI_COLOR_TOPBAR, 0);
    lv_obj_set_style_border_width(bar->root, 0, 0);
    lv_obj_set_style_radius(bar->root, 0, 0);
    lv_obj_clear_flag(bar->root, LV_OBJ_FLAG_SCROLLABLE);

    bar->wifi_label = lv_label_create(bar->root);
    lv_label_set_text(bar->wifi_label, "WiFi");
    lv_obj_set_style_text_font(bar->wifi_label, UI_FONT_NORMAL, 0);
    lv_obj_set_pos(bar->wifi_label, 8, 7);

    bar->datetime_label = lv_label_create(bar->root);
    lv_label_set_text(bar->datetime_label, "--/--/---- --:--");
    lv_obj_set_style_text_font(bar->datetime_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(bar->datetime_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_size(bar->datetime_label, 220, 14);
    lv_obj_set_pos(bar->datetime_label, 130, 7);
    lv_obj_set_style_text_align(bar->datetime_label, LV_TEXT_ALIGN_CENTER, 0);

    bar->feed_btn = lv_btn_create(bar->root);
    lv_obj_set_size(bar->feed_btn, 90, 24);
    lv_obj_set_pos(bar->feed_btn, 382, 4);
    lv_obj_set_style_radius(bar->feed_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_set_style_bg_color(bar->feed_btn, UI_COLOR_WARN, 0);
    lv_obj_add_event_cb(bar->feed_btn, feed_btn_cb, LV_EVENT_CLICKED, NULL);

    bar->feed_label = lv_label_create(bar->feed_btn);
    lv_label_set_text(bar->feed_label, "FEED");
    lv_obj_center(bar->feed_label);
    lv_obj_set_style_text_font(bar->feed_label, UI_FONT_NORMAL, 0);
}

void ui_topbar_update(ui_topbar_t *bar, const ui_topbar_vm_t *vm)
{
    if (vm->wifi_ok) {
        lv_label_set_text(bar->wifi_label, "WiFi");
        lv_obj_set_style_text_color(bar->wifi_label, UI_COLOR_OK, 0);
    } else {
        lv_label_set_text(bar->wifi_label, "WiFi");
        lv_obj_set_style_text_color(bar->wifi_label, UI_COLOR_TEXT_DIM, 0);
    }

    lv_label_set_text(bar->datetime_label, vm->datetime_text);

    if (vm->feed_mode_active) {
        lv_obj_set_style_bg_color(bar->feed_btn, lv_color_hex(0xFF6B00), 0);
        lv_obj_add_state(bar->feed_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_set_style_bg_color(bar->feed_btn, UI_COLOR_WARN, 0);
        lv_obj_clear_state(bar->feed_btn, LV_STATE_DISABLED);
    }
}
