// @requirement RF-UI-MENU-002 Hub de configuracoes (SRS §73.1)
#ifndef HMI_SCREENS_UI_SCREEN_CONFIG_HUB_H
#define HMI_SCREENS_UI_SCREEN_CONFIG_HUB_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_config_hub_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_config_hub_update(ui_root_vm_t *vm);

#endif
