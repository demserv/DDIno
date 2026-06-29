// @requirement RF-THERMAL-001 Tela de dashboard
#ifndef HMI_SCREENS_UI_SCREEN_DASHBOARD_H
#define HMI_SCREENS_UI_SCREEN_DASHBOARD_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_dashboard_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_dashboard_update(ui_root_vm_t *vm);

#endif
