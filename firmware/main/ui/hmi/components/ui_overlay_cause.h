// @requirement RF-UI-OVERLAY-001.1 Templates por causa SAFE_OFF/EMERGENCY
#ifndef HMI_COMPONENTS_UI_OVERLAY_CAUSE_H
#define HMI_COMPONENTS_UI_OVERLAY_CAUSE_H

#include "system_types.h"

typedef struct {
    const char *title;
    const char *occurred;
    const char *impact;
    const char *action;
    const char *exit_hint;
} ui_overlay_cause_template_t;

const ui_overlay_cause_template_t *ui_overlay_cause_lookup(safeoff_reason_t reason);

#endif
