// @requirement RF-ENERGY-001 a RF-ENERGY-010 Tela de energia com V, A, W, PF, Hz
#include "lvgl.h"

#include "../ui_screens.h"
#include "driver_acs712.h"
#include "driver_pzem.h"
#include "global_state.h"

static lv_obj_t *v_label = NULL;
static lv_obj_t *a_label = NULL;
static lv_obj_t *w_label = NULL;
static lv_obj_t *pf_label = NULL;
static lv_obj_t *hz_label = NULL;
static lv_obj_t *chart = NULL;
static lv_chart_series_t *chart_series = NULL;
static lv_coord_t chart_data[12] = {0};

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

static void screen_init_energy(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Energia");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    // Instant metrics in 2 rows x 3 cols
    v_label = lv_label_create(parent);
    lv_label_set_text(v_label, "T: --- V");
    lv_obj_align(v_label, LV_ALIGN_TOP_LEFT, 10, 68);
    lv_obj_set_style_text_font(v_label, &lv_font_montserrat_14, 0);

    a_label = lv_label_create(parent);
    lv_label_set_text(a_label, "I: -.-- A");
    lv_obj_align(a_label, LV_ALIGN_TOP_LEFT, 170, 68);
    lv_obj_set_style_text_font(a_label, &lv_font_montserrat_14, 0);

    w_label = lv_label_create(parent);
    lv_label_set_text(w_label, "P: --- W");
    lv_obj_align(w_label, LV_ALIGN_TOP_LEFT, 330, 68);
    lv_obj_set_style_text_font(w_label, &lv_font_montserrat_14, 0);

    pf_label = lv_label_create(parent);
    lv_label_set_text(pf_label, "PF: -.--");
    lv_obj_align(pf_label, LV_ALIGN_TOP_LEFT, 10, 92);
    lv_obj_set_style_text_font(pf_label, &lv_font_montserrat_14, 0);

    hz_label = lv_label_create(parent);
    lv_label_set_text(hz_label, "F: -- Hz");
    lv_obj_align(hz_label, LV_ALIGN_TOP_LEFT, 170, 92);
    lv_obj_set_style_text_font(hz_label, &lv_font_montserrat_14, 0);

    // Bar chart: kWh monthly (placeholder data)
    chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 460, 150);
    lv_obj_align(chart, LV_ALIGN_TOP_LEFT, 10, 120);
    lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(chart, 12);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_bg_color(chart, lv_color_make(15, 15, 35), 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_radius(chart, 4, 0);

    lv_chart_set_div_line_count(chart, 4, 0);

    chart_series = lv_chart_add_series(chart, lv_color_make(0, 140, 220), LV_CHART_AXIS_PRIMARY_Y);

    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 5, 1, true, 40);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, 12, 1, true, 30);

    lv_chart_set_ext_y_array(chart, chart_series, chart_data);

    // Month labels
    static const char *months[] = {"J","F","M","A","M","J","J","A","S","O","N","D"};
    for (int i = 0; i < 12; i++) {
        lv_obj_t *ml = lv_label_create(chart);
        lv_label_set_text(ml, months[i]);
        lv_obj_set_style_text_color(ml, lv_color_make(150, 150, 150), 0);
        lv_obj_align(ml, LV_ALIGN_BOTTOM_LEFT, 6 + i * 38, 0);
    }
}

static void screen_update_energy(void)
{
    char buf[64];

    float v = g_pzem.voltage_v;
    if (v < 1.0f) v = 127.0f;
    snprintf(buf, sizeof(buf), "T: %.1f V", (double)v);
    lv_label_set_text(v_label, buf);

    float total_i = 0;
    for (int i = 0; i < 10; i++) {
        float cur = 0;
        acs712_read_plug((uint8_t)(i + 1), &cur);
        total_i += cur;
    }
    total_i += g_pzem.current_a;
    snprintf(buf, sizeof(buf), "I: %.2f A", (double)total_i);
    lv_label_set_text(a_label, buf);

    snprintf(buf, sizeof(buf), "P: %.0f W", (double)g_pzem.power_w);
    lv_label_set_text(w_label, buf);

    snprintf(buf, sizeof(buf), "PF: %.2f", (double)g_pzem.pf);
    lv_label_set_text(pf_label, buf);

    snprintf(buf, sizeof(buf), "F: %.1f Hz", (double)g_pzem.frequency_hz);
    lv_label_set_text(hz_label, buf);

    // Update chart with energy data (placeholder kWh pattern)
    for (int i = 0; i < 12; i++) {
        chart_data[i] = (lv_coord_t)(20 + (i * 7) % 40);
    }
    lv_chart_set_ext_y_array(chart, chart_series, chart_data);
}

static void __attribute__((constructor)) register_energy(void)
{
    ui_screen_register(SCREEN_ENERGY, screen_init_energy, screen_update_energy);
}

