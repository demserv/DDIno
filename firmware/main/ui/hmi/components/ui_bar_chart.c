#include "ui_bar_chart.h"
#include "../ui_theme.h"
#include <stdio.h>
#include <string.h>

void ui_bar_chart_create(lv_obj_t *parent, int x, int y, int w, int h, const ui_energy_vm_t *energy)
{
    lv_obj_t *chart = lv_obj_create(parent);
    lv_obj_set_size(chart, w, h);
    lv_obj_set_pos(chart, x, y);
    lv_obj_set_style_bg_color(chart, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(chart, UI_RADIUS_CARD, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(chart, (void *)energy);

    /* check if all data invalid */
    int valid_count = 0;
    float max_kwh = 0;
    for (int i = 0; i < UI_ENERGY_MONTHS; i++) {
        if (energy->monthly_data_valid[i]) {
            valid_count++;
            if (energy->monthly_kwh[i] > max_kwh) {
                max_kwh = energy->monthly_kwh[i];
            }
        }
    }

    if (valid_count == 0) {
        lv_obj_t *na = lv_label_create(chart);
        lv_label_set_text(na, "Dados indisponiveis");
        lv_obj_set_style_text_font(na, UI_FONT_NORMAL, 0);
        lv_obj_set_style_text_color(na, UI_COLOR_TEXT_DIM, 0);
        lv_obj_center(na);
        return;
    }

    int bar_w = (w - 40) / UI_ENERGY_MONTHS;
    if (bar_w > 40) bar_w = 40;
    int chart_h_usable = h - 30;
    int bar_gap = ((w - 40) - (bar_w * UI_ENERGY_MONTHS)) / (UI_ENERGY_MONTHS - 1);
    if (bar_gap < 2) bar_gap = 2;

    for (int i = 0; i < UI_ENERGY_MONTHS; i++) {
        int bx = 20 + i * (bar_w + bar_gap);

        lv_obj_t *bar = lv_obj_create(chart);
        int bh = 0;
        if (energy->monthly_data_valid[i] && max_kwh > 0) {
            bh = (int)((energy->monthly_kwh[i] / max_kwh) * (chart_h_usable - 10));
            if (bh < 4) bh = 4;
        }
        int by = chart_h_usable - bh;
        lv_obj_set_size(bar, bar_w, bh);
        lv_obj_set_pos(bar, bx, by);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

        if (energy->monthly_data_valid[i]) {
            lv_obj_set_style_bg_color(bar, UI_COLOR_CYAN, 0);
        } else {
            lv_obj_set_style_bg_color(bar, UI_COLOR_DISABLED, 0);
        }

        lv_obj_t *label = lv_label_create(chart);
        if (energy->monthly_data_valid[i]) {
            char lb[16];
            snprintf(lb, sizeof(lb), "%.0f", (double)energy->monthly_kwh[i]);
            lv_label_set_text(label, lb);
            lv_obj_set_style_text_color(label, UI_COLOR_TEXT_MAIN, 0);
        } else {
            lv_label_set_text(label, "N/D");
            lv_obj_set_style_text_color(label, UI_COLOR_TEXT_DIM, 0);
        }
        lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
        lv_obj_align_to(label, bar, LV_ALIGN_OUT_TOP_MID, 0, -2);

        lv_obj_t *ml = lv_label_create(chart);
        lv_label_set_text(ml, energy->month_labels[i]);
        lv_obj_set_style_text_font(ml, UI_FONT_SMALL, 0);
        lv_obj_set_style_text_color(ml, UI_COLOR_TEXT_DIM, 0);
        lv_obj_align_to(ml, bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    }
}

void ui_bar_chart_update(lv_obj_t *chart_root, const ui_energy_vm_t *energy)
{
    (void)chart_root;
    (void)energy;
    /* TODO: reconstruir barras quando dados mudarem */
}
