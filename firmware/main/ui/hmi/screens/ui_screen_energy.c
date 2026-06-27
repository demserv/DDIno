// @requirement RF-ENERGY-001 a RF-ENERGY-010 Tela de energia com 4 metric cards + gráfico mensal
// @requirement RF-ENERGY-004 Projeção mensal mensal
#include "ui_screen_energy.h"
#include "../ui_theme.h"
#include "../components/ui_metric_card.h"
#include "../components/ui_bar_chart.h"
#include <stdio.h>

static lv_obj_t *power_label = NULL;
static ui_metric_card_t card_voltage;
static ui_metric_card_t card_current;
static ui_metric_card_t card_freq;
static ui_metric_card_t card_pf;

void ui_screen_energy_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Energia");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 205, 6);

    power_label = lv_label_create(parent);
    lv_label_set_text(power_label, "--- W");
    lv_obj_set_style_text_font(power_label, UI_FONT_BIG, 0);
    lv_obj_set_style_text_color(power_label, UI_COLOR_YELLOW, 0);
    lv_obj_set_pos(power_label, 142, 34);

    ui_metric_card_create(&card_voltage, parent, 25, 86, 100, 28, "Tensao");
    ui_metric_card_create(&card_current, parent, 135, 86, 100, 28, "Corrente");
    ui_metric_card_create(&card_freq, parent, 245, 86, 100, 28, "Frequencia");
    ui_metric_card_create(&card_pf, parent, 355, 86, 100, 28, "F.P.");

    lv_obj_t *chart_title = lv_label_create(parent);
    lv_label_set_text(chart_title, "Consumo Mensal (kWh)");
    lv_obj_set_style_text_font(chart_title, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(chart_title, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(chart_title, 20, 124);

    ui_bar_chart_create(parent, 20, 145, 440, 110, &vm->energy);

    ui_screen_energy_update(vm);
}

void ui_screen_energy_update(ui_root_vm_t *vm)
{
    if (!vm) return;
    char buf[32];

    snprintf(buf, sizeof(buf), "\xe2\x9a\xa1 %.0f W", (double)vm->energy.power_w);
    lv_label_set_text(power_label, buf);

    snprintf(buf, sizeof(buf), "%.1f", (double)vm->energy.voltage_v);
    ui_metric_card_set_value(&card_voltage, buf, "V");
    if (vm->energy.voltage_v > 0) {
        ui_metric_card_set_color(&card_voltage, UI_COLOR_CYAN);
    }

    snprintf(buf, sizeof(buf), "%.2f", (double)vm->energy.current_a);
    ui_metric_card_set_value(&card_current, buf, "A");
    if (vm->energy.current_a > 0) {
        ui_metric_card_set_color(&card_current, UI_COLOR_OK);
    }

    snprintf(buf, sizeof(buf), "%.1f", (double)vm->energy.frequency_hz);
    ui_metric_card_set_value(&card_freq, buf, "Hz");
    if (vm->energy.frequency_hz > 0) {
        ui_metric_card_set_color(&card_freq, UI_COLOR_INFO);
    }

    snprintf(buf, sizeof(buf), "%.2f", (double)vm->energy.power_factor);
    ui_metric_card_set_value(&card_pf, buf, "");
    if (vm->energy.power_factor > 0) {
        ui_metric_card_set_color(&card_pf, UI_COLOR_PURPLE);
    }
}
