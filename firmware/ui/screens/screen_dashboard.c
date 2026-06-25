#include "../ui_screens.h"
#include "global_state.h"
#include "plug_model.h"
#include "driver_pzem.h"
#include "driver_ds18b20.h"
#include "services/config_manager.h"
#include "lvgl.h"

static lv_obj_t *temp_value = NULL;
static lv_obj_t *trend_label = NULL;
static lv_obj_t *setpoint_label = NULL;
static lv_obj_t *thermal_state_label = NULL;
static lv_obj_t *ato_label = NULL;
static lv_obj_t *ato_bar = NULL;
static lv_obj_t *w_label = NULL;
static lv_obj_t *v_label = NULL;
static lv_obj_t *a_label = NULL;
static lv_obj_t *pf_label = NULL;

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

static void screen_init_dashboard(lv_obj_t *parent)
{
    // Left section: Temperature
    temp_value = lv_label_create(parent);
    lv_label_set_text(temp_value, "--.- C");
    lv_obj_set_style_text_font(temp_value, &lv_font_montserrat_20, 0);
    lv_obj_align(temp_value, LV_ALIGN_TOP_LEFT, 15, 50);

    trend_label = lv_label_create(parent);
    lv_label_set_text(trend_label, "---");
    lv_obj_align(trend_label, LV_ALIGN_TOP_LEFT, 15, 85);
    lv_obj_set_style_text_font(trend_label, &lv_font_montserrat_14, 0);

    setpoint_label = lv_label_create(parent);
    lv_label_set_text(setpoint_label, "Setpoint: --.- C");
    lv_obj_align(setpoint_label, LV_ALIGN_TOP_LEFT, 15, 110);
    lv_obj_set_style_text_font(setpoint_label, &lv_font_montserrat_14, 0);

    thermal_state_label = lv_label_create(parent);
    lv_label_set_text(thermal_state_label, "---");
    lv_obj_align(thermal_state_label, LV_ALIGN_TOP_LEFT, 15, 135);
    lv_obj_set_style_text_font(thermal_state_label, &lv_font_montserrat_16, 0);

    // Right section: ATO + Electrical
    ato_label = lv_label_create(parent);
    lv_label_set_text(ato_label, "ATO: --");
    lv_obj_align(ato_label, LV_ALIGN_TOP_LEFT, 260, 50);
    lv_obj_set_style_text_font(ato_label, &lv_font_montserrat_14, 0);

    ato_bar = lv_bar_create(parent);
    lv_obj_set_size(ato_bar, 200, 16);
    lv_obj_align(ato_bar, LV_ALIGN_TOP_LEFT, 260, 75);
    lv_bar_set_range(ato_bar, 0, 100);
    lv_bar_set_value(ato_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ato_bar, lv_color_make(60, 60, 60), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ato_bar, lv_color_make(0, 100, 200), LV_PART_INDICATOR);

    v_label = lv_label_create(parent);
    lv_label_set_text(v_label, "Tensao: --- V");
    lv_obj_align(v_label, LV_ALIGN_TOP_LEFT, 260, 105);
    lv_obj_set_style_text_font(v_label, &lv_font_montserrat_14, 0);

    a_label = lv_label_create(parent);
    lv_label_set_text(a_label, "Corrente: -.-- A");
    lv_obj_align(a_label, LV_ALIGN_TOP_LEFT, 260, 130);
    lv_obj_set_style_text_font(a_label, &lv_font_montserrat_14, 0);

    w_label = lv_label_create(parent);
    lv_label_set_text(w_label, "Potencia: --- W");
    lv_obj_align(w_label, LV_ALIGN_TOP_LEFT, 260, 155);
    lv_obj_set_style_text_font(w_label, &lv_font_montserrat_14, 0);

    pf_label = lv_label_create(parent);
    lv_label_set_text(pf_label, "Fator P: -.--");
    lv_obj_align(pf_label, LV_ALIGN_TOP_LEFT, 260, 180);
    lv_obj_set_style_text_font(pf_label, &lv_font_montserrat_14, 0);

    // Bottom hint
    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< > Navegar | Enter Menu");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_text_color(hint, lv_color_make(120, 120, 120), 0);
}

static void screen_update_dashboard(void)
{
    float temp = 25.0f;
    ds18b20_read(&temp);
    char buf[64];

    // Temperature
    snprintf(buf, sizeof(buf), "%.1f C", (double)temp);
    lv_label_set_text(temp_value, buf);
    if (temp > 30.0f) {
        lv_obj_set_style_text_color(temp_value, lv_color_make(255, 50, 50), 0);
    } else if (temp > 26.0f) {
        lv_obj_set_style_text_color(temp_value, lv_color_make(255, 200, 50), 0);
    } else {
        lv_obj_set_style_text_color(temp_value, lv_color_make(100, 200, 255), 0);
    }

    // Setpoint (from config)
    const thermal_params_storage_t *tp = config_get_thermal();
    snprintf(buf, sizeof(buf), "Setpoint: %.1f C", (double)tp->temp_normal_c);
    lv_label_set_text(setpoint_label, buf);

    // Trend + Thermal state
    float diff = temp - tp->temp_normal_c;
    if (diff > tp->hysteresis_c) {
        snprintf(buf, sizeof(buf), "RESFRIANDO");
        lv_label_set_text(thermal_state_label, buf);
        lv_obj_set_style_text_color(thermal_state_label, lv_color_make(100, 180, 255), 0);
        lv_label_set_text(trend_label, "->");
    } else if (diff < -tp->hysteresis_c) {
        lv_label_set_text(thermal_state_label, "AQUECENDO");
        lv_obj_set_style_text_color(thermal_state_label, lv_color_make(255, 50, 50), 0);
        lv_label_set_text(trend_label, "<-");
    } else {
        lv_label_set_text(thermal_state_label, "ESTAVEL");
        lv_obj_set_style_text_color(thermal_state_label, lv_color_make(0, 200, 0), 0);
        lv_label_set_text(trend_label, "---");
    }

    // ATO
    snprintf(buf, sizeof(buf), "ATO: %s", g_gs.ato_ok ? "OK" : "FALHA");
    lv_label_set_text(ato_label, buf);
    lv_obj_set_style_text_color(ato_label,
        g_gs.ato_ok ? lv_color_make(0, 200, 0) : lv_color_make(255, 50, 50), 0);
    lv_bar_set_value(ato_bar, g_gs.ato_ok ? 75 : 0, LV_ANIM_OFF);

    // Electrical
    snprintf(buf, sizeof(buf), "Tensao: %.1f V", (double)g_pzem.voltage_v);
    lv_label_set_text(v_label, buf);
    snprintf(buf, sizeof(buf), "Corrente: %.2f A", (double)g_pzem.current_a);
    lv_label_set_text(a_label, buf);
    snprintf(buf, sizeof(buf), "Potencia: %.0f W", (double)g_pzem.power_w);
    lv_label_set_text(w_label, buf);
    snprintf(buf, sizeof(buf), "Fator P: %.2f", (double)g_pzem.pf);
    lv_label_set_text(pf_label, buf);
}

static void __attribute__((constructor)) register_dashboard(void)
{
    ui_screen_register(SCREEN_DASHBOARD, screen_init_dashboard, screen_update_dashboard);
}
