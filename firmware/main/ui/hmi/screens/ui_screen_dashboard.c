// @requirement RF-THERMAL-001 a RF-THERMAL-009 Dashboard com temperatura, setpoint, estado térmico
// @requirement RF-ENERGY-001 a RF-ENERGY-010 Indicadores de energia no dashboard
#include "ui_screen_dashboard.h"
#include "../ui_theme.h"
#include <stdio.h>

static lv_obj_t *temp_value = NULL;
static lv_obj_t *setpoint_label = NULL;
static lv_obj_t *thermal_state_label = NULL;
static lv_obj_t *ato_label = NULL;
static lv_obj_t *power_label = NULL;
static lv_obj_t *electric_line = NULL;

void ui_screen_dashboard_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Temperatura Atual");
    lv_obj_set_style_text_font(title, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(title, 174, 6);

    temp_value = lv_label_create(parent);
    lv_label_set_text(temp_value, "--.- C");
    lv_obj_set_style_text_font(temp_value, UI_FONT_HUGE, 0);
    lv_obj_set_style_text_color(temp_value, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(temp_value, 126, 27);

    setpoint_label = lv_label_create(parent);
    lv_label_set_text(setpoint_label, "Setpoint: --.- C");
    lv_obj_set_style_text_font(setpoint_label, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(setpoint_label, UI_COLOR_CYAN, 0);
    lv_obj_set_pos(setpoint_label, 156, 84);

    thermal_state_label = lv_label_create(parent);
    lv_label_set_text(thermal_state_label, "IDLE");
    lv_obj_set_style_text_font(thermal_state_label, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(thermal_state_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(thermal_state_label, 32, 115);

    ato_label = lv_label_create(parent);
    lv_label_set_text(ato_label, "ATO: --");
    lv_obj_set_style_text_font(ato_label, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(ato_label, UI_COLOR_BLUE, 0);
    lv_obj_set_pos(ato_label, 292, 118);

    power_label = lv_label_create(parent);
    lv_label_set_text(power_label, "--- W");
    lv_obj_set_style_text_font(power_label, UI_FONT_BIG, 0);
    lv_obj_set_style_text_color(power_label, UI_COLOR_YELLOW, 0);
    lv_obj_set_pos(power_label, 30, 158);

    electric_line = lv_label_create(parent);
    lv_label_set_text(electric_line, "---V | ---A | PF ---");
    lv_obj_set_style_text_font(electric_line, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(electric_line, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(electric_line, 170, 164);

    ui_screen_dashboard_update(vm);
}

void ui_screen_dashboard_update(ui_root_vm_t *vm)
{
    if (!vm) return;

    char buf[64];

    snprintf(buf, sizeof(buf), "%.1f\xc2\xb0" "C", (double)vm->dashboard.current_temp_c);
    lv_label_set_text(temp_value, buf);

    snprintf(buf, sizeof(buf), "Setpoint: %.1f\xc2\xb0" "C", (double)vm->dashboard.setpoint_temp_c);
    lv_label_set_text(setpoint_label, buf);

    switch (vm->dashboard.thermal_state) {
        case UI_THERMAL_IDLE: lv_label_set_text(thermal_state_label, "IDLE"); break;
        case UI_THERMAL_HEATING: lv_label_set_text(thermal_state_label, "AQUECENDO"); break;
        case UI_THERMAL_COOLING: lv_label_set_text(thermal_state_label, "RESFRIANDO"); break;
        case UI_THERMAL_ALERT: lv_label_set_text(thermal_state_label, "ALERTA"); break;
        case UI_THERMAL_CRITICAL: lv_label_set_text(thermal_state_label, "CRITICO"); break;
    }

    lv_label_set_text(ato_label, vm->dashboard.ato_text);

    snprintf(buf, sizeof(buf), "\xe2\x9a\xa1 %.0f W", (double)vm->dashboard.power_w);
    lv_label_set_text(power_label, buf);

    snprintf(buf, sizeof(buf), "%.0fV | %.2fA | PF %.2f",
             (double)vm->dashboard.voltage_v,
             (double)vm->dashboard.current_a,
             (double)vm->dashboard.power_factor);
    lv_label_set_text(electric_line, buf);
}
