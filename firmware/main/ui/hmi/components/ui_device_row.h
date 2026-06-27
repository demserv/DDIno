// @requirement RF-PLUG-001 Linha de dispositivo com dados de plugue
#pragma once

#include "lvgl.h"
#include "../ui_view_model.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *icon_label;
    lv_obj_t *code_label;
    lv_obj_t *name_label;
    lv_obj_t *tag_label;
    lv_obj_t *state_label;
    lv_obj_t *current_label;
} ui_device_row_t;

void ui_device_row_create(ui_device_row_t *row, lv_obj_t *parent, int x, int y);
void ui_device_row_update(ui_device_row_t *row, const ui_plug_vm_t *plug);
