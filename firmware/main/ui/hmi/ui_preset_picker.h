// @requirement RF-PLUG-001 Seletor de preset para P03-P10
#ifndef HMI_UI_PRESET_PICKER_H
#define HMI_UI_PRESET_PICKER_H

#include "lvgl.h"
#include "plug_model.h"

void ui_preset_picker_show(lv_obj_t *parent, plug_id_t plug_id);

#endif
