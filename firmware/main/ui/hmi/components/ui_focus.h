// @requirement RF-UI-INPUT-001 Foco visual para navegação por teclado
#ifndef HMI_COMPONENTS_UI_FOCUS_H
#define HMI_COMPONENTS_UI_FOCUS_H

#include "lvgl.h"

void ui_focus_apply(lv_obj_t *obj);
void ui_focus_clear(lv_obj_t *obj);

#endif
