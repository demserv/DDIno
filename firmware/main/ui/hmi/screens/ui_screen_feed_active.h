// @requirement RF-FEED-002 Tela de Feed Mode ativo
#ifndef HMI_SCREENS_UI_SCREEN_FEED_ACTIVE_H
#define HMI_SCREENS_UI_SCREEN_FEED_ACTIVE_H

#include "lvgl.h"
#include "../ui_view_model.h"

void ui_screen_feed_active_create(lv_obj_t *parent, ui_root_vm_t *vm);
void ui_screen_feed_active_update(ui_root_vm_t *vm);

#endif
