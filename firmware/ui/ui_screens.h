// @requirement RF-UI-CAROUSEL-001 Carrossel automático
#ifndef FIRMWARE_UI_SCREENS_H
#define FIRMWARE_UI_SCREENS_H

#include "esp_err.h"
#include "lvgl.h"
#include "system_types.h"

#define SCREEN_COUNT 9

typedef enum {
    SCREEN_DASHBOARD = 0,
    SCREEN_DEVICES1,
    SCREEN_DEVICES2,
    SCREEN_ENERGY,
    SCREEN_MENU,
    SCREEN_SUBMENU,
    SCREEN_ALERTS,
    SCREEN_DIAGNOSTIC,
    SCREEN_CALIBRATION
} screen_id_t;

esp_err_t ui_screens_init(void);
void ui_screen_show(screen_id_t id);
screen_id_t ui_screen_current(void);
void ui_screen_next(void);
void ui_screen_prev(void);
void ui_screen_update_all(void);
void ui_screen_notify_activity(void);
void ui_toggle_mute(void);
bool ui_is_muted(void);

typedef void (*screen_init_fn_t)(lv_obj_t *);
typedef void (*screen_update_fn_t)(void);

void ui_screen_register(screen_id_t id, screen_init_fn_t init, screen_update_fn_t update);

void ui_carousel_enable(bool en);
void ui_carousel_set_interval(uint32_t interval_ms);

#endif
