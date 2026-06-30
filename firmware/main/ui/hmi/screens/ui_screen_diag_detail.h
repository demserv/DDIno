#ifndef HMI_SCREENS_UI_SCREEN_DIAG_DETAIL_H
#define HMI_SCREENS_UI_SCREEN_DIAG_DETAIL_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_diag_detail_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_diag_detail_update(ui_root_vm_t *vm);
void ui_screen_diag_detail_show_subsystem(int subsystem_index);

#endif
