// @requirement RNF-CALIB-001 Calibração assistida de sensores
#include "ui_screen_calibration.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "config_manager.h"
#include <stdio.h>

static lv_obj_t *g_calib_labels[12];

static void calib_back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_DIAGNOSTICS);
}

void ui_screen_calibration_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Calibracao");
    lv_obj_set_style_text_font(title, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    const int col1_x = 10;
    const int col2_x = 240;
    const int row_h = 22;

    for (int i = 0; i < 12; i++) {
        int col = (i < 6) ? col1_x : col2_x;
        int row = i % 6;
        g_calib_labels[i] = lv_label_create(parent);
        lv_label_set_text(g_calib_labels[i], "--");
        lv_obj_set_style_text_font(g_calib_labels[i], UI_FONT_NORMAL, 0);
        lv_obj_set_style_text_color(g_calib_labels[i], UI_COLOR_TEXT_DIM, 0);
        lv_obj_align(g_calib_labels[i], LV_ALIGN_TOP_LEFT, col, 40 + row * row_h);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "Valores via API REST / wizard | Voltar: Diagnostico");
    lv_obj_set_style_text_font(hint, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -40);

    lv_obj_t *back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 100, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_set_style_bg_color(back_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_add_event_cb(back_btn, calib_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Voltar");
    lv_obj_center(back_lbl);
}

void ui_screen_calibration_update(ui_root_vm_t *vm)
{
    (void)vm;
    const calibration_params_storage_t *cp = config_get_calibration();
    if (!cp) return;

    char buf[64];
    static const char *prefix[] = {
        "ATO Zero", "Temp Off", "P01 Zero", "P02 Zero", "P03 Zero", "P04 Zero",
        "P05 Zero", "P06 Zero", "P07 Zero", "P08 Zero", "P09 Zero", "P10 Zero"
    };

    snprintf(buf, sizeof(buf), "%s: %ld ADC", prefix[0], (long)cp->ato_zero_offset_adc);
    lv_label_set_text(g_calib_labels[0], buf);

    snprintf(buf, sizeof(buf), "%s: %+.1f C", prefix[1], (double)cp->temp_offset_c);
    lv_label_set_text(g_calib_labels[1], buf);

    for (int i = 0; i < 10; i++) {
        snprintf(buf, sizeof(buf), "%s: %.0f mV", prefix[i + 2], (double)cp->acs712_zero_offset_mv[i]);
        lv_label_set_text(g_calib_labels[i + 2], buf);
    }
}
