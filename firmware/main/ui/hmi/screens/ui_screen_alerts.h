// @requirement RF-ALERT-001 Tela de alertas
#ifndef HMI_SCREENS_UI_SCREEN_ALERTS_H
#define HMI_SCREENS_UI_SCREEN_ALERTS_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_alerts_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_alerts_update(ui_root_vm_t *vm);

#endif
