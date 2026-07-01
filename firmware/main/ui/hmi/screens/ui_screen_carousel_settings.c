// @requirement RF-UI-CAROUSEL-001.1 Carrossel: intervalo + pausa pos interacao
#include "ui_screen_carousel_settings.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "config_manager.h"
#include <stdio.h>

static lv_obj_t *g_interval_lbl = NULL;
static lv_obj_t *g_pause_lbl = NULL;
static lv_obj_t *g_hint_lbl = NULL;

static const uint16_t s_interval_opts[] = {0, 15, 30, 60};
#define INTERVAL_COUNT 4

static const uint16_t s_pause_opts[] = {5, 10, 15, 30};
#define PAUSE_COUNT 4

static int find_index(const uint16_t *opts, int count, uint16_t val)
{
    for (int i = 0; i < count; i++) {
        if (opts[i] == val) return i;
    }
    return 0;
}

static void refresh_labels(void)
{
    uint16_t interval_s = config_get_carousel_interval_s();
    uint16_t pause_s = config_get_carousel_pause_s();

    if (g_interval_lbl) {
        if (interval_s == 0) {
            lv_label_set_text(g_interval_lbl, "Desativado");
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u segundos", (unsigned)interval_s);
            lv_label_set_text(g_interval_lbl, buf);
        }
    }
    if (g_pause_lbl) {
        char pbuf[32];
        snprintf(pbuf, sizeof(pbuf), "%u segundos", (unsigned)pause_s);
        lv_label_set_text(g_pause_lbl, pbuf);
    }
    if (g_hint_lbl) {
        char hint[80];
        snprintf(hint, sizeof(hint),
                 "Padrao: 15s intervalo, 5s pausa apos toque.");
        lv_label_set_text(g_hint_lbl, hint);
    }
}

static void cycle_interval_cb(lv_event_t *e)
{
    (void)e;
    uint16_t cur = config_get_carousel_interval_s();
    int idx = find_index(s_interval_opts, INTERVAL_COUNT, cur);
    idx = (idx + 1) % INTERVAL_COUNT;
    if (config_set_carousel_interval_s(s_interval_opts[idx]) == ESP_OK) {
        ui_screen_manager_carousel_apply_config();
        refresh_labels();
    }
}

static void cycle_pause_cb(lv_event_t *e)
{
    (void)e;
    uint16_t cur = config_get_carousel_pause_s();
    int idx = find_index(s_pause_opts, PAUSE_COUNT, cur);
    idx = (idx + 1) % PAUSE_COUNT;
    if (config_set_carousel_pause_s(s_pause_opts[idx]) == ESP_OK) {
        refresh_labels();
    }
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_CONFIG_HUB);
}

void ui_screen_carousel_settings_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Carrossel Automatico");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_pos(title, 130, 6);

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 440, 110);
    lv_obj_set_pos(panel, 20, 36);
    lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(panel, UI_RADIUS_CARD, 0);

    lv_obj_t *ilbl = lv_label_create(panel);
    lv_label_set_text(ilbl, "Intervalo entre telas:");
    lv_obj_set_pos(ilbl, 12, 8);

    g_interval_lbl = lv_label_create(panel);
    lv_obj_set_style_text_color(g_interval_lbl, UI_COLOR_CYAN, 0);
    lv_obj_set_pos(g_interval_lbl, 12, 28);

    lv_obj_t *plbl = lv_label_create(panel);
    lv_label_set_text(plbl, "Pausa apos interacao:");
    lv_obj_set_pos(plbl, 12, 58);

    g_pause_lbl = lv_label_create(panel);
    lv_obj_set_style_text_color(g_pause_lbl, UI_COLOR_CYAN, 0);
    lv_obj_set_pos(g_pause_lbl, 12, 78);

    g_hint_lbl = lv_label_create(parent);
    lv_obj_set_width(g_hint_lbl, 440);
    lv_obj_set_style_text_font(g_hint_lbl, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_hint_lbl, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(g_hint_lbl, 20, 152);

    lv_obj_t *cycle_int = lv_btn_create(parent);
    lv_obj_set_size(cycle_int, 200, 32);
    lv_obj_set_pos(cycle_int, 20, 180);
    lv_obj_add_event_cb(cycle_int, cycle_interval_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cil = lv_label_create(cycle_int);
    lv_label_set_text(cil, "Intervalo OFF/15/30/60");
    lv_obj_center(cil);

    lv_obj_t *cycle_pause = lv_btn_create(parent);
    lv_obj_set_size(cycle_pause, 200, 32);
    lv_obj_set_pos(cycle_pause, 240, 180);
    lv_obj_add_event_cb(cycle_pause, cycle_pause_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cpl = lv_label_create(cycle_pause);
    lv_label_set_text(cpl, "Pausa 5/10/15/30s");
    lv_obj_center(cpl);

    lv_obj_t *back = lv_btn_create(parent);
    lv_obj_set_size(back, 100, 28);
    lv_obj_set_pos(back, 10, 250);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "Voltar");
    lv_obj_center(bl);

    refresh_labels();
}

void ui_screen_carousel_settings_update(ui_root_vm_t *vm)
{
    (void)vm;
    refresh_labels();
}
