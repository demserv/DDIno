// @requirement RF-THERMAL-003 a RF-THERMAL-007 Tela de configuração de parâmetros térmicos
// @requirement RF-THERMAL-006 Parâmetros térmicos obrigatórios definidos pelo usuário
#include "ui_screen_config_temperature.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include "../ui_screen_manager.h"
#include <stdio.h>

typedef struct {
    lv_obj_t *label;
    lv_obj_t *value;
} config_row_t;

static config_row_t rows[6];

static void save_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_SAVE_THERMAL_CONFIG);
}

static void cancel_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
}

void ui_screen_config_temperature_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    lv_obj_t *breadcrumb = lv_label_create(parent);
    lv_label_set_text(breadcrumb, "< Configuracao > Temperatura");
    lv_obj_set_style_text_font(breadcrumb, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(breadcrumb, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(breadcrumb, 14, 6);

    static const char *row_labels[6] = {"Setpoint", "Temp. Minima", "Temp. Maxima", "Temp. Critica", "Histerese", "Temp. Extrema"};
    static const int y_pos[6] = {36, 70, 104, 138, 172, 206};
    for (int i = 0; i < 6; i++) {
        lv_obj_t *panel = lv_obj_create(parent);
        lv_obj_set_size(panel, 460, 32);
        lv_obj_set_pos(panel, 10, y_pos[i]);
        lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
        lv_obj_set_style_radius(panel, UI_RADIUS_CARD, 0);
        lv_obj_set_style_border_width(panel, 0, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        rows[i].label = lv_label_create(panel);
        lv_label_set_text(rows[i].label, row_labels[i]);
        lv_obj_set_style_text_font(rows[i].label, UI_FONT_NORMAL, 0);
        lv_obj_set_style_text_color(rows[i].label, UI_COLOR_TEXT_MAIN, 0);
        lv_obj_set_pos(rows[i].label, 22, 7);

        rows[i].value = lv_label_create(panel);
        lv_label_set_text(rows[i].value, "--.-");
        lv_obj_set_style_text_font(rows[i].value, UI_FONT_NORMAL, 0);
        lv_obj_set_style_text_color(rows[i].value, UI_COLOR_CYAN, 0);
        lv_obj_set_pos(rows[i].value, 410, 7);
    }

    lv_obj_t *save_btn = lv_btn_create(parent);
    lv_obj_set_size(save_btn, 120, 26);
    lv_obj_set_pos(save_btn, 110, 236);
    lv_obj_set_style_bg_color(save_btn, UI_COLOR_OK, 0);
    lv_obj_set_style_radius(save_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Salvar");
    lv_obj_center(save_label);
    lv_obj_add_event_cb(save_btn, save_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel_btn = lv_btn_create(parent);
    lv_obj_set_size(cancel_btn, 120, 26);
    lv_obj_set_pos(cancel_btn, 250, 236);
    lv_obj_set_style_bg_color(cancel_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(cancel_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancelar");
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(cancel_btn, cancel_btn_cb, LV_EVENT_CLICKED, NULL);

    ui_screen_config_temperature_update(vm);
}

void ui_screen_config_temperature_update(ui_root_vm_t *vm)
{
    if (!vm) return;
    float vals[6] = {
        vm->config_temperature.setpoint_c,
        vm->config_temperature.temp_min_c,
        vm->config_temperature.temp_max_c,
        vm->config_temperature.temp_critical_c,
        vm->config_temperature.hysteresis_c,
        vm->config_temperature.temp_extreme_c
    };
    for (int i = 0; i < 6; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f\xc2\xb0" "C", (double)vals[i]);
        lv_label_set_text(rows[i].value, buf);
    }
}
