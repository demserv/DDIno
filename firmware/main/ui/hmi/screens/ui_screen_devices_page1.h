// @requirement RF-PLUG-001 Tela dispositivos página 1
#pragma once

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_devices_page1_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_devices_page1_update(ui_root_vm_t *vm);
