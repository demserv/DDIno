#include "ui_screen_feed_active.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include <stdio.h>

static lv_obj_t *timer_label = NULL;
static lv_obj_t *exit_btn = NULL;

static void exit_feed_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_EXIT_FEED_MODE);
}

void ui_screen_feed_active_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "FEED MODE ATIVO");
    lv_obj_set_style_text_font(title, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_WARN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    timer_label = lv_label_create(parent);
    lv_label_set_text(timer_label, "Tempo restante: 00:00");
    lv_obj_set_style_text_font(timer_label, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(timer_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(timer_label, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *pause_title = lv_label_create(parent);
    lv_label_set_text(pause_title, "Bombas pausadas:");
    lv_obj_set_style_text_font(pause_title, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(pause_title, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(pause_title, LV_ALIGN_TOP_LEFT, 30, 70);

    static const char *paused_plugs[] = {
        "P03 Bomba Principal",
        "P05 Bomba Skimmer",
        "P07 Bomba Dosadora",
        "P08 Bomba Circulacao"
    };
    int py = 94;
    for (int i = 0; i < 4; i++) {
        lv_obj_t *pp = lv_label_create(parent);
        lv_label_set_text(pp, paused_plugs[i]);
        lv_obj_set_style_text_font(pp, UI_FONT_NORMAL, 0);
        lv_obj_set_style_text_color(pp, UI_COLOR_TEXT_MAIN, 0);
        lv_obj_set_pos(pp, 46, py);
        py += 20;
    }

    lv_obj_t *notice = lv_label_create(parent);
    lv_label_set_text(notice, "P01/P02 nao participam do Feed Mode.");
    lv_obj_set_style_text_font(notice, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(notice, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(notice, LV_ALIGN_BOTTOM_LEFT, 14, -50);

    exit_btn = lv_btn_create(parent);
    lv_obj_set_size(exit_btn, 120, 28);
    lv_obj_align(exit_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(exit_btn, UI_COLOR_CRITICAL, 0);
    lv_obj_set_style_radius(exit_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(exit_btn, exit_feed_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *exit_lbl = lv_label_create(exit_btn);
    lv_label_set_text(exit_lbl, "Sair do Feed Mode");
    lv_obj_center(exit_lbl);
    lv_obj_set_style_text_font(exit_lbl, UI_FONT_NORMAL, 0);
}

void ui_screen_feed_active_update(ui_root_vm_t *vm)
{
    if (!vm) return;
    char buf[32];
    uint32_t remaining = vm->topbar.feed_remaining_s;
    uint32_t mins = remaining / 60;
    uint32_t secs = remaining % 60;
    snprintf(buf, sizeof(buf), "Tempo restante: %02u:%02u", (unsigned)mins, (unsigned)secs);
    lv_label_set_text(timer_label, buf);
}
