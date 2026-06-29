#ifndef HMI_UI_STATUS_BAR_H
#define HMI_UI_STATUS_BAR_H

#include "lvgl.h"
#include "ui_view_model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;
    lv_obj_t *state_label;
    lv_obj_t *wifi_icon;
    lv_obj_t *temp_label;
    lv_obj_t *alert_count_label;
} ui_status_bar_t;

void ui_status_bar_create(ui_status_bar_t *bar, lv_obj_t *parent);
void ui_status_bar_update(ui_status_bar_t *bar, const ui_topbar_vm_t *vm);

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /* HMI_UI_STATUS_BAR_H */