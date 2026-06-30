// @requirement RF-RESET-004 Diálogo de confirmação para ações críticas
#ifndef HMI_COMPONENTS_UI_CONFIRM_DIALOG_H
#define HMI_COMPONENTS_UI_CONFIRM_DIALOG_H

#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *message_label;
    lv_obj_t *confirm_btn;
    lv_obj_t *cancel_btn;
    lv_obj_t *confirm_label;
    lv_obj_t *cancel_label;
    bool result;
    bool active;
} ui_confirm_dialog_t;

void ui_confirm_dialog_create(ui_confirm_dialog_t *dlg, lv_obj_t *parent, const char *message);
void ui_confirm_dialog_set_confirm_cb(ui_confirm_dialog_t *dlg, lv_event_cb_t cb);
void ui_confirm_dialog_set_cancel_cb(ui_confirm_dialog_t *dlg, lv_event_cb_t cb);
void ui_confirm_dialog_show(ui_confirm_dialog_t *dlg, bool show);

#endif
