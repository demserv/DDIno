// @requirement RF-UI-CAROUSEL-001 Menu principal
#ifndef HMI_SCREENS_UI_SCREEN_MAIN_MENU_H
#define HMI_SCREENS_UI_SCREEN_MAIN_MENU_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_main_menu_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_main_menu_update(ui_root_vm_t *vm);

#endif
