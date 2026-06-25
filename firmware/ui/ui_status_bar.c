// @requirement RF-UI-STATUS-001 Barra de status persistente
#include "ui_status_bar.h"
#include "ui_screens.h"
#include "global_state.h"
#include "system_types.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui_status_bar";

static lv_obj_t *bar_cont = NULL;
static lv_obj_t *left_label = NULL;
static lv_obj_t *center_label = NULL;
static lv_obj_t *right_label = NULL;

extern global_state_t g_gs;

esp_err_t ui_status_bar_init(void)
{
    ESP_LOGI(TAG, "Initializing status bar");

    bar_cont = lv_obj_create(lv_layer_top());
    lv_obj_set_size(bar_cont, 480, 30);
    lv_obj_set_style_radius(bar_cont, 0, 0);
    lv_obj_set_style_border_width(bar_cont, 0, 0);
    lv_obj_set_style_pad_all(bar_cont, 0, 0);
    lv_obj_clear_flag(bar_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(bar_cont, LV_ALIGN_TOP_MID, 0, 0);

    left_label = lv_label_create(bar_cont);
    lv_obj_align(left_label, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_text_font(left_label, &lv_font_montserrat_14, 0);

    center_label = lv_label_create(bar_cont);
    lv_obj_align(center_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(center_label, &lv_font_montserrat_14, 0);

    right_label = lv_label_create(bar_cont);
    lv_obj_align(right_label, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_set_style_text_font(right_label, &lv_font_montserrat_14, 0);

    lv_obj_move_foreground(bar_cont);

    return ESP_OK;
}

void ui_status_bar_update(void)
{
    if (!bar_cont) return;

    const char *state_name = "";
    const char *dot = "";
    lv_color_t bg = lv_color_make(0, 40, 0);
    lv_color_t dot_color = lv_color_make(0, 200, 0);

    switch (g_gs.system_state) {
        case SYSTEM_STATE_NORMAL:
            state_name = "NORMAL";
            dot = "\xe2\x97\x8f";
            dot_color = lv_color_make(0, 200, 0);
            bg = lv_color_make(0, 40, 0);
            break;
        case SYSTEM_STATE_DEGRADED:
            state_name = "DEGRAD";
            dot = "\xe2\x97\x8f";
            dot_color = lv_color_make(200, 180, 0);
            bg = lv_color_make(40, 36, 0);
            break;
        case SYSTEM_STATE_SAFE_OFF:
            state_name = "SAFE_OFF";
            dot = "\xe2\x97\x8f";
            dot_color = lv_color_make(200, 50, 0);
            bg = lv_color_make(40, 0, 0);
            break;
        case SYSTEM_STATE_EMERGENCY:
            state_name = "EMERG";
            dot = "\xe2\x97\x8f";
            dot_color = lv_color_make(200, 0, 0);
            bg = lv_color_make(50, 0, 0);
            break;
        default:
            break;
    }

    lv_obj_set_style_bg_color(bar_cont, bg, 0);

    char left_buf[48];
    snprintf(left_buf, sizeof(left_buf), "%s %s", dot, state_name);
    lv_label_set_text(left_label, left_buf);
    lv_obj_set_style_text_color(left_label, dot_color, 0);

    char center_buf[48];
    center_buf[0] = '\0';
    if (!g_gs.wizard_completed) {
        strcat(center_buf, "WIZ ");
    }
    if (g_gs.feed_active) {
        strcat(center_buf, "FEED ");
    }
    if (ui_is_muted()) {
        strcat(center_buf, "MUTE ");
    }
    size_t clen = strlen(center_buf);
    if (clen > 0) {
        center_buf[clen - 1] = '\0';
    }
    lv_label_set_text(center_label, center_buf);
    lv_obj_set_style_text_color(center_label, lv_color_make(255, 255, 255), 0);

    uint64_t uptime = g_gs.uptime_s;
    uint64_t h = uptime / 3600;
    uint64_t m = (uptime % 3600) / 60;
    uint64_t s = uptime % 60;
    char right_buf[64];
    snprintf(right_buf, sizeof(right_buf), "%s %s  %lluh %02llum %02llus",
             g_gs.sd_ok ? "SD" : "sd",
             g_gs.wifi_ok ? "WiFi" : "wifi",
             (unsigned long long)h, (unsigned long long)m, (unsigned long long)s);
    lv_label_set_text(right_label, right_buf);
    lv_obj_set_style_text_color(right_label, lv_color_make(200, 200, 200), 0);

    lv_obj_move_foreground(bar_cont);
}
