// @requirement RF-PLUG-001 Tela dispositivos página 2
#ifndef HMI_SCREENS_UI_SCREEN_DEVICES_PAGE2_H
#define HMI_SCREENS_UI_SCREEN_DEVICES_PAGE2_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_devices_page2_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_devices_page2_update(ui_root_vm_t *vm);

#endif
