// @requirement RF-ALERT-001 Linha de alerta com ícone, texto, severidade
#pragma once

#include "lvgl.h"
#include "../ui_view_model.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *side_bar;
    lv_obj_t *id_label;
    lv_obj_t *severity_label;
    lv_obj_t *timestamp_label;
    lv_obj_t *message_label;
    lv_obj_t *ack_label;
    lv_obj_t *action_label;
} ui_alert_row_t;

void ui_alert_row_create(ui_alert_row_t *row, lv_obj_t *parent, int x, int y);
void ui_alert_row_update(ui_alert_row_t *row, const ui_alert_vm_t *alert);
