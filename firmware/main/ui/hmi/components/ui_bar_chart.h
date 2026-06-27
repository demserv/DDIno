// @requirement RF-ENERGY-004 Componente de gráfico de barras
#pragma once

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_bar_chart_create(lv_obj_t *parent, int x, int y, int w, int h, const ui_energy_vm_t *energy);
void ui_bar_chart_update(lv_obj_t *chart_root, const ui_energy_vm_t *energy);
