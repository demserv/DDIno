// @requirement RF-THERMAL-006 Parametros termicos configuraveis via UI
#include "../ui_screens.h"
#include "global_state.h"
#include "services/config_manager.h"
#include "lvgl.h"

static lv_obj_t *setpoint_label = NULL;
static lv_obj_t *temp_min_label = NULL;
static lv_obj_t *temp_max_label = NULL;
static lv_obj_t *temp_crit_label = NULL;
static lv_obj_t *hysteresis_label = NULL;
static lv_obj_t *temp_extreme_label = NULL;

extern global_state_t g_gs;

static void toggle_param(int idx)
{
    thermal_params_storage_t tp = *config_get_thermal();
    switch (idx) {
        case 0: tp.temp_normal_c += 0.5f; if (tp.temp_normal_c > 35.0f) tp.temp_normal_c = 20.0f; break;
        case 1: tp.temp_normal_c -= 0.5f; if (tp.temp_normal_c < 20.0f) tp.temp_normal_c = 35.0f; break;
        case 2: tp.temp_critical_c += 1.0f; if (tp.temp_critical_c > 45.0f) tp.temp_critical_c = 30.0f; break;
        case 3: tp.temp_critical_c -= 1.0f; if (tp.temp_critical_c < 30.0f) tp.temp_critical_c = 45.0f; break;
        case 4: tp.temp_extreme_c += 1.0f; if (tp.temp_extreme_c > 50.0f) tp.temp_extreme_c = 35.0f; break;
        case 5: tp.temp_extreme_c -= 1.0f; if (tp.temp_extreme_c < 35.0f) tp.temp_extreme_c = 50.0f; break;
        case 6: tp.hysteresis_c += 0.5f; if (tp.hysteresis_c > 5.0f) tp.hysteresis_c = 0.5f; break;
        case 7: tp.hysteresis_c -= 0.5f; if (tp.hysteresis_c < 0.5f) tp.hysteresis_c = 5.0f; break;
    }
    config_set_thermal(&tp);
}

static void btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    intptr_t idx = (intptr_t)lv_obj_get_user_data(btn);
    toggle_param((int)idx);
}

static lv_obj_t *create_param_row(lv_obj_t *parent, const char *name, int y, int idx_inc, int idx_dec)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 460, 32);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, y);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 3, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *nl = lv_label_create(row);
    lv_label_set_text(nl, name);
    lv_obj_align(nl, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_14, 0);

    lv_obj_t *vl = lv_label_create(row);
    lv_label_set_text(vl, "--.-");
    lv_obj_align(vl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(vl, &lv_font_montserrat_14, 0);

    lv_obj_t *inc = lv_btn_create(row);
    lv_obj_set_size(inc, 32, 26);
    lv_obj_align(inc, LV_ALIGN_RIGHT_MID, -40, 0);
    lv_obj_set_user_data(inc, (void *)(intptr_t)idx_inc);
    lv_obj_add_event_cb(inc, btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *il = lv_label_create(inc);
    lv_label_set_text(il, "+");
    lv_obj_center(il);

    lv_obj_t *dec = lv_btn_create(row);
    lv_obj_set_size(dec, 32, 26);
    lv_obj_align(dec, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_user_data(dec, (void *)(intptr_t)idx_dec);
    lv_obj_add_event_cb(dec, btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dl = lv_label_create(dec);
    lv_label_set_text(dl, "-");
    lv_obj_center(dl);

    return vl;
}

static void screen_init_submenu(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Config. Temperatura");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    setpoint_label = create_param_row(parent, "Setpoint", 70, 0, 1);
    temp_min_label = create_param_row(parent, "Temp. Normal", 106, 0, 1);
    temp_max_label = create_param_row(parent, "Temp. Critica", 142, 2, 3);
    temp_crit_label = create_param_row(parent, "Temp. Extrema", 178, 4, 5);
    hysteresis_label = create_param_row(parent, "Histerese", 214, 6, 7);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< > Voltar | +/- p/ alterar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -42);
    lv_obj_set_style_text_color(hint, lv_color_make(120, 120, 120), 0);
}

static void screen_update_submenu(void)
{
    const thermal_params_storage_t *tp = config_get_thermal();
    char buf[16];
    if (setpoint_label) {
        snprintf(buf, sizeof(buf), "%.1f", (double)tp->temp_normal_c);
        lv_label_set_text(setpoint_label, buf);
    }
    if (temp_min_label) {
        snprintf(buf, sizeof(buf), "%.1f", (double)tp->temp_normal_c);
        lv_label_set_text(temp_min_label, buf);
    }
    if (temp_max_label) {
        snprintf(buf, sizeof(buf), "%.1f", (double)tp->temp_critical_c);
        lv_label_set_text(temp_max_label, buf);
    }
    if (temp_crit_label) {
        snprintf(buf, sizeof(buf), "%.1f", (double)tp->temp_extreme_c);
        lv_label_set_text(temp_crit_label, buf);
    }
    if (hysteresis_label) {
        snprintf(buf, sizeof(buf), "%.1f", (double)tp->hysteresis_c);
        lv_label_set_text(hysteresis_label, buf);
    }
}

static void __attribute__((constructor)) register_submenu(void)
{
    ui_screen_register(SCREEN_SUBMENU, screen_init_submenu, screen_update_submenu);
}
