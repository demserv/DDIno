// @requirement RF-UI-STATUS-001 Footer persistente
#ifndef HMI_COMPONENTS_UI_FOOTER_H
#define HMI_COMPONENTS_UI_FOOTER_H

#include "lvgl.h"
#include "../ui_view_model.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *state_label;
    lv_obj_t *uptime_label;
    lv_obj_t *alerts_label;
    lv_obj_t *page_label;
} ui_footer_t;

void ui_footer_create(ui_footer_t *footer, lv_obj_t *parent);
void ui_footer_update(ui_footer_t *footer, const ui_footer_vm_t *vm);

#endif
