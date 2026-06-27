// @requirement RF-GLOBAL-003 Badge de estado sempre visível na UI
#include "ui_state_badge.h"
#include "global_state.h"
#include "system_types.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui_state_badge";

static lv_obj_t *badge_cont = NULL;
static lv_obj_t *badge_icon = NULL;
static lv_obj_t *badge_label = NULL;

extern global_state_t g_gs;

esp_err_t ui_state_badge_init(void)
{
    ESP_LOGI(TAG, "Initializing state badge");

    badge_cont = lv_obj_create(lv_layer_top());
    lv_obj_set_size(badge_cont, 90, 28);
    lv_obj_set_style_border_width(badge_cont, 1, 0);
    lv_obj_set_style_border_color(badge_cont, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_radius(badge_cont, 4, 0);
    lv_obj_set_style_pad_all(badge_cont, 2, 0);
    lv_obj_align(badge_cont, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_clear_flag(badge_cont, LV_OBJ_FLAG_SCROLLABLE);

    badge_icon = lv_label_create(badge_cont);
    lv_obj_align(badge_icon, LV_ALIGN_LEFT_MID, 2, 0);

    badge_label = lv_label_create(badge_cont);
    lv_obj_align(badge_label, LV_ALIGN_RIGHT_MID, -2, 0);

    return ESP_OK;
}

void ui_state_badge_update(void)
{
    if (!badge_cont) return;

    const char *text = "";
    const char *icon = "";
    lv_color_t bg = lv_color_make(0, 128, 0);

    switch (g_gs.system_state) {
        case SYSTEM_STATE_NORMAL:
            text = "NORMAL";
            icon = "\xe2\x9c\x93";
            bg = lv_color_make(0, 128, 0);
            break;
        case SYSTEM_STATE_DEGRADED:
            text = "DEGRAD.";
            icon = "\xe2\x9a\xa0";
            bg = lv_color_make(200, 180, 0);
            break;
        case SYSTEM_STATE_SAFE_OFF:
            text = "SAFE OFF";
            icon = "\xe2\x9b\x94";
            bg = lv_color_make(180, 0, 0);
            break;
        case SYSTEM_STATE_EMERGENCY:
            text = "EMERG.";
            icon = "\xf0\x9f\x9a\xa8";
            bg = lv_color_make(200, 0, 0);
            break;
        default:
            break;
    }

    lv_obj_set_style_bg_color(badge_cont, bg, 0);

    lv_label_set_text(badge_icon, icon);
    lv_label_set_text(badge_label, text);

    if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF || g_gs.system_state == SYSTEM_STATE_EMERGENCY) {
        lv_obj_set_style_text_color(badge_icon, lv_color_make(255, 255, 255), 0);
        lv_obj_set_style_text_color(badge_label, lv_color_make(255, 255, 255), 0);
    } else {
        lv_obj_set_style_text_color(badge_icon, lv_color_make(255, 255, 255), 0);
        lv_obj_set_style_text_color(badge_label, lv_color_make(255, 255, 255), 0);
    }

    lv_obj_move_foreground(badge_cont);
}
