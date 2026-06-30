// @requirement RF-UI-DIAG-002 Drill-down de self-test por subsistema
#include "ui_screen_diag_detail.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "services/self_test.h"
#include <stdio.h>

static lv_obj_t *g_list = NULL;

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_DIAGNOSTICS);
}

void ui_screen_diag_detail_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Self-test Detalhado");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 14, 6);

    g_list = lv_obj_create(parent);
    lv_obj_set_size(g_list, 460, 220);
    lv_obj_set_pos(g_list, 10, 32);
    lv_obj_set_style_border_width(g_list, 0, 0);
    lv_obj_set_style_bg_color(g_list, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(g_list, UI_RADIUS_CARD, 0);

    lv_obj_t *back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 90, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -12);
    lv_obj_set_style_bg_color(back_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_add_event_cb(back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Voltar");
    lv_obj_center(back_lbl);

    ui_screen_diag_detail_update(vm);
}

void ui_screen_diag_detail_update(ui_root_vm_t *vm)
{
    (void)vm;
    if (!g_list) return;
    lv_obj_clean(g_list);

    int y = 4;
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        const selftest_result_t *r = self_test_get_result((selftest_id_t)i);
        if (!r) continue;
        char line[96];
        snprintf(line, sizeof(line), "%s: %s %s",
                 r->name, self_test_status_str(r->status),
                 r->detail[0] ? r->detail : "");
        lv_obj_t *lbl = lv_label_create(g_list);
        lv_label_set_text(lbl, line);
        lv_obj_set_style_text_font(lbl, UI_FONT_SMALL, 0);
        lv_obj_set_pos(lbl, 8, y);
        y += 16;
        if (y > 200) break;
    }
}
