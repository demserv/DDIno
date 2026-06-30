// @requirement RF-UI-STATUS-001 Topbar com informações de sistema
#ifndef HMI_COMPONENTS_UI_TOPBAR_H
#define HMI_COMPONENTS_UI_TOPBAR_H

#include "lvgl.h"
#include "../ui_view_model.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *wifi_label;
    lv_obj_t *datetime_label;
    lv_obj_t *feed_btn;
    lv_obj_t *feed_label;
} ui_topbar_t;

void ui_topbar_create(ui_topbar_t *bar, lv_obj_t *parent);
void ui_topbar_update(ui_topbar_t *bar, const ui_topbar_vm_t *vm);

#endif
