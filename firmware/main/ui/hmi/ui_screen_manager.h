// @requirement RF-UI-CAROUSEL-001 Gerenciador de telas
#pragma once

#include "lvgl.h"
#include "ui_view_model.h"

typedef enum {
    UI_SCREEN_DASHBOARD = 0,
    UI_SCREEN_DEVICES_1,
    UI_SCREEN_DEVICES_2,
    UI_SCREEN_ENERGY,
    UI_SCREEN_MAIN_MENU,
    UI_SCREEN_CONFIG_TEMPERATURE,
    UI_SCREEN_ALERTS,
    UI_SCREEN_FEED_ACTIVE,
    UI_SCREEN_DIAGNOSTICS,
    UI_SCREEN_SYSTEM,
    UI_SCREEN_LOGS,
    UI_SCREEN_WIZARD
} ui_screen_id_t;

void ui_screen_manager_init(lv_obj_t *root, ui_root_vm_t *vm);
void ui_screen_manager_show(ui_screen_id_t screen_id);
void ui_screen_manager_refresh(void);
ui_screen_id_t ui_screen_manager_get_current(void);
