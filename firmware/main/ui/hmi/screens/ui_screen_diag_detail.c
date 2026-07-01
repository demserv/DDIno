// @requirement RF-UI-DIAG-002 Drill-down de self-test por subsistema
#include "ui_screen_diag_detail.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "self_test.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *g_list = NULL;
static lv_obj_t *g_title = NULL;
static int g_subsystem = -1;

static const char *SUBSYSTEM_TITLES[] = {
    "Temperatura", "ATO", "Energia", "Plugues",
    "Persistencia", "Seguranca", "Self-test",
    "Barramentos", "I/O"
};

static bool test_matches_subsystem(selftest_id_t id, int sub)
{
    switch (sub) {
    case 0: return id == SELFTEST_ID_DS18B20;
    case 1: return id == SELFTEST_ID_ATO_SENSOR;
    case 2: return id == SELFTEST_ID_PZEM || id == SELFTEST_ID_ACS712;
    case 3: return id >= SELFTEST_ID_RELAY_P01 && id <= SELFTEST_ID_RELAY_P10;
    case 4: return id == SELFTEST_ID_NVS || id == SELFTEST_ID_SPI_SD;
    case 5: return id == SELFTEST_ID_HEAP || id == SELFTEST_ID_WDT;
    case 6: return true;
    case 7: return id == SELFTEST_ID_I2C_MCP23017 || id == SELFTEST_ID_SPI_MCP3208 ||
                 id == SELFTEST_ID_MCP3208_CH2;
    case 8: return id == SELFTEST_ID_AD_KEYPAD || id == SELFTEST_ID_SPI_DISPLAY ||
                 id == SELFTEST_ID_TOUCH || id == SELFTEST_ID_RTC;
    default: return true;
    }
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_DIAGNOSTICS);
}

void ui_screen_diag_detail_show_subsystem(int subsystem_index)
{
    if (subsystem_index < 0 || subsystem_index > 8) {
        g_subsystem = 6;
    } else {
        g_subsystem = subsystem_index;
    }
}

void ui_screen_diag_detail_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    g_title = lv_label_create(parent);
    lv_label_set_text(g_title, "Self-test Detalhado");
    lv_obj_set_style_text_font(g_title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(g_title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(g_title, 14, 6);

    g_list = lv_obj_create(parent);
    lv_obj_set_size(g_list, 460, 220);
    lv_obj_set_pos(g_list, 10, 32);
    lv_obj_set_style_border_width(g_list, 0, 0);
    lv_obj_set_style_bg_color(g_list, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(g_list, UI_RADIUS_CARD, 0);
    lv_obj_add_flag(g_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(g_list, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 90, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -12);
    lv_obj_set_style_bg_color(back_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_add_event_cb(back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Voltar");
    lv_obj_center(back_lbl);

    if (g_subsystem < 0) {
        g_subsystem = 6;
    }
    ui_screen_diag_detail_update(vm);
}

void ui_screen_diag_detail_update(ui_root_vm_t *vm)
{
    (void)vm;
    if (!g_list) return;
    lv_obj_clean(g_list);

    if (g_title && g_subsystem >= 0 && g_subsystem <= 8) {
        char title[64];
        snprintf(title, sizeof(title), "Self-test: %s", SUBSYSTEM_TITLES[g_subsystem]);
        lv_label_set_text(g_title, title);
    }

    int shown = 0;
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        if (g_subsystem >= 0 && !test_matches_subsystem((selftest_id_t)i, g_subsystem)) {
            continue;
        }
        const selftest_result_t *r = self_test_get_result((selftest_id_t)i);
        if (!r) continue;
        char line[96];
        snprintf(line, sizeof(line), "%s: %s %s",
                 r->name, self_test_status_str(r->status),
                 r->detail[0] ? r->detail : "");
        lv_obj_t *lbl = lv_label_create(g_list);
        lv_label_set_text(lbl, line);
        lv_obj_set_style_text_font(lbl, UI_FONT_SMALL, 0);
        lv_obj_set_width(lbl, 440);
        lv_obj_set_pos(lbl, 8, 4 + shown * 16);
        shown++;
    }

    if (shown == 0) {
        lv_obj_t *lbl = lv_label_create(g_list);
        lv_label_set_text(lbl, "Nenhum teste neste subsistema");
        lv_obj_set_style_text_font(lbl, UI_FONT_SMALL, 0);
        lv_obj_set_pos(lbl, 8, 4);
    }
}
