/* @requirement RNF-USAB-002 hint inline de navegação */
#pragma once
#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *label;
} ui_inline_hint_t;

void ui_inline_hint_create(ui_inline_hint_t *hint, lv_obj_t *parent);
void ui_inline_hint_show(ui_inline_hint_t *hint, const char *text, uint32_t ms);
void ui_inline_hint_hide(ui_inline_hint_t *hint);
