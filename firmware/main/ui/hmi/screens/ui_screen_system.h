// @requirement RF-RESET-001 Tela de sistema
#ifndef HMI_SCREENS_UI_SCREEN_SYSTEM_H
#define HMI_SCREENS_UI_SCREEN_SYSTEM_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_system_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_system_update(ui_root_vm_t *vm);

#endif
