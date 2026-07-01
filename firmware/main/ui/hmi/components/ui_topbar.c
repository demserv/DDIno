// @requirement RF-UI-STATUS-001 Topbar com WiFi, datetime, state badge, SD, selftest, alert count, botão FEED
#include "ui_topbar.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include <stdio.h>
#include <string.h>

static void feed_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_FEED_MODE);
}

static lv_color_t state_badge_color(ui_system_state_t st)
{
    switch (st) {
        case UI_SYSTEM_EMERGENCY: return UI_COLOR_CRITICAL;
        case UI_SYSTEM_SAFE_OFF:  return UI_COLOR_HIGH;
        case UI_SYSTEM_DEGRADED:  return UI_COLOR_WARN;
        default:                  return UI_COLOR_OK;
    }
}

static const char *state_badge_text(ui_system_state_t st)
{
    switch (st) {
        case UI_SYSTEM_EMERGENCY: return "EMERG";
        case UI_SYSTEM_SAFE_OFF:  return "SAFE";
        case UI_SYSTEM_DEGRADED:  return "DEG";
        default:                  return "OK";
    }
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
    lv_obj_set_pos(bar->wifi_label, 4, 7);

    ui_status_badge_create(&bar->state_badge, bar->root, 40, 5, 50, 22);
    ui_status_badge_set_text(&bar->state_badge, "OK");
    ui_status_badge_set_color(&bar->state_badge, UI_COLOR_OK);

    bar->datetime_label = lv_label_create(bar->root);
    lv_label_set_text(bar->datetime_label, "--/--/---- --:--");
    lv_obj_set_style_text_font(bar->datetime_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(bar->datetime_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_size(bar->datetime_label, 170, 14);
    lv_obj_set_pos(bar->datetime_label, 98, 7);
    lv_obj_set_style_text_align(bar->datetime_label, LV_TEXT_ALIGN_CENTER, 0);

    bar->sd_label = lv_label_create(bar->root);
    lv_label_set_text(bar->sd_label, "SD");
    lv_obj_set_style_text_font(bar->sd_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(bar->sd_label, 276, 9);

    bar->selftest_label = lv_label_create(bar->root);
    lv_label_set_text(bar->selftest_label, "TST");
    lv_obj_set_style_text_font(bar->selftest_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(bar->selftest_label, 304, 9);

    bar->badge_maint_label = lv_label_create(bar->root);
    lv_label_set_text(bar->badge_maint_label, "");
    lv_obj_set_style_text_font(bar->badge_maint_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(bar->badge_maint_label, 330, 9);

    bar->badge_mute_label = lv_label_create(bar->root);
    lv_label_set_text(bar->badge_mute_label, "");
    lv_obj_set_style_text_font(bar->badge_mute_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(bar->badge_mute_label, 355, 9);

    bar->badge_wiz_label = lv_label_create(bar->root);
    lv_label_set_text(bar->badge_wiz_label, "");
    lv_obj_set_style_text_font(bar->badge_wiz_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(bar->badge_wiz_label, 375, 9);

    bar->nvs_label = lv_label_create(bar->root);
    lv_label_set_text(bar->nvs_label, "");
    lv_obj_set_style_text_font(bar->nvs_label, UI_FONT_SMALL, 0);
    lv_obj_set_pos(bar->nvs_label, 395, 9);

    bar->alert_count_label = lv_label_create(bar->root);
    lv_label_set_text(bar->alert_count_label, "!");
    lv_obj_set_style_text_font(bar->alert_count_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(bar->alert_count_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(bar->alert_count_label, 340, 9);

    bar->feed_btn = lv_btn_create(bar->root);
    lv_obj_set_size(bar->feed_btn, 80, 24);
    lv_obj_set_pos(bar->feed_btn, 392, 4);
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

    ui_status_badge_set_text(&bar->state_badge, state_badge_text(vm->system_state));
    ui_status_badge_set_color(&bar->state_badge, state_badge_color(vm->system_state));

    lv_label_set_text(bar->datetime_label, vm->datetime_text);

    if (vm->sd_ok) {
        lv_label_set_text(bar->sd_label, "SD");
        lv_obj_set_style_text_color(bar->sd_label, UI_COLOR_OK, 0);
    } else {
        lv_label_set_text(bar->sd_label, "SD");
        lv_obj_set_style_text_color(bar->sd_label, UI_COLOR_CRITICAL, 0);
    }

    if (vm->selftest_passed) {
        lv_label_set_text(bar->selftest_label, "TST");
        lv_obj_set_style_text_color(bar->selftest_label, UI_COLOR_OK, 0);
    } else {
        lv_label_set_text(bar->selftest_label, "TST");
        lv_obj_set_style_text_color(bar->selftest_label, UI_COLOR_WARN, 0);
    }

    if (vm->alert_count > 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u!", (unsigned)vm->alert_count);
        lv_label_set_text(bar->alert_count_label, buf);
        lv_obj_set_style_text_color(bar->alert_count_label, UI_COLOR_CRITICAL, 0);
    } else {
        lv_label_set_text(bar->alert_count_label, "");
    }

    if (vm->maintenance_mode) {
        lv_label_set_text(bar->badge_maint_label, "MNT");
        lv_obj_set_style_text_color(bar->badge_maint_label, UI_COLOR_WARN, 0);
    } else {
        lv_label_set_text(bar->badge_maint_label, "");
    }

    if (vm->mute_active) {
        lv_label_set_text(bar->badge_mute_label, "MUTE");
        lv_obj_set_style_text_color(bar->badge_mute_label, UI_COLOR_TEXT_DIM, 0);
    } else {
        lv_label_set_text(bar->badge_mute_label, "");
    }

    if (vm->wizard_incomplete) {
        lv_label_set_text(bar->badge_wiz_label, "WIZ");
        lv_obj_set_style_text_color(bar->badge_wiz_label, UI_COLOR_WARN, 0);
    } else {
        lv_label_set_text(bar->badge_wiz_label, "");
    }

    if (!vm->nvs_ok) {
        lv_label_set_text(bar->nvs_label, "NVS!");
        lv_obj_set_style_text_color(bar->nvs_label, UI_COLOR_CRITICAL, 0);
    } else {
        lv_label_set_text(bar->nvs_label, "");
    }

    if (vm->feed_mode_active) {
        lv_obj_set_style_bg_color(bar->feed_btn, lv_color_hex(0xFF6B00), 0);
        lv_obj_add_state(bar->feed_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_set_style_bg_color(bar->feed_btn, UI_COLOR_WARN, 0);
        lv_obj_clear_state(bar->feed_btn, LV_STATE_DISABLED);
    }
}
