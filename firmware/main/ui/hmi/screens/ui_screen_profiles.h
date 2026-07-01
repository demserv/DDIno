// @requirement RF-UI-MENU-002 / RF-UI-MENU-002.1 Perfis CRUD + export/import
#ifndef HMI_SCREENS_UI_SCREEN_PROFILES_H
#define HMI_SCREENS_UI_SCREEN_PROFILES_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_profiles_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_profiles_update(ui_root_vm_t *vm);

#endif
