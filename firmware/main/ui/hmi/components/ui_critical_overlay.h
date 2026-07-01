// @requirement RF-UI-OVERLAY-001 Overlay crítico SAFE_OFF/EMERGENCY
#ifndef HMI_COMPONENTS_UI_CRITICAL_OVERLAY_H
#define HMI_COMPONENTS_UI_CRITICAL_OVERLAY_H

#include "lvgl.h"
#include "../ui_view_model.h"
#include "system_types.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *title_label;
    lv_obj_t *message_label;
    lv_obj_t *alerts_btn;
    lv_obj_t *diag_btn;
    lv_obj_t *ack_btn;   /* @requirement ACK inline direto no overlay crítico */
    bool visible;
} ui_critical_overlay_t;

void ui_critical_overlay_safeoff_create(ui_critical_overlay_t *ov, lv_obj_t *parent);
void ui_critical_overlay_emergency_create(ui_critical_overlay_t *ov, lv_obj_t *parent);
void ui_critical_overlay_show(ui_critical_overlay_t *ov, bool show);
void ui_critical_overlay_set_cause(ui_critical_overlay_t *ov, safeoff_reason_t reason, bool emergency);

#endif
