// @requirement RF-THERMAL-006 Tela de configuração térmica
#pragma once

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_config_temperature_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_config_temperature_update(ui_root_vm_t *vm);
