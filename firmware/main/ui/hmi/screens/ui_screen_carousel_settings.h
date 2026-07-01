// @requirement RF-UI-CAROUSEL-001.1 Tempo de carrossel configurável
#ifndef HMI_SCREENS_UI_SCREEN_CAROUSEL_SETTINGS_H
#define HMI_SCREENS_UI_SCREEN_CAROUSEL_SETTINGS_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_carousel_settings_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_carousel_settings_update(ui_root_vm_t *vm);

#endif
