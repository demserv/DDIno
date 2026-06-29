// @requirement RF-UI-STATUS-001 Footer grafico com status geral, uptime e alertas
#include "ui_footer.h"
#include "global_state.h"
#include "hardware_config.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "ui_footer";

static lv_obj_t *footer_cont = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *uptime_label = NULL;
static lv_obj_t *alert_count_label = NULL;

extern global_state_t g_gs;

esp_err_t ui_footer_init(void)
{
    ESP_LOGI(TAG, "Initializing graphical footer");

    footer_cont = lv_obj_create(lv_layer_top());
    lv_obj_set_size(footer_cont, 480, 35);
    lv_obj_set_style_radius(footer_cont, 0, 0);
    lv_obj_set_style_border_width(footer_cont, 0, 0);
    lv_obj_set_style_pad_all(footer_cont, 0, 0);
    lv_obj_clear_flag(footer_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(footer_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer_cont, lv_color_make(10, 10, 30), 0);

    status_label = lv_label_create(footer_cont);
    lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);

    uptime_label = lv_label_create(footer_cont);
    lv_obj_align(uptime_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(uptime_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(uptime_label, lv_color_make(180, 180, 180), 0);

    alert_count_label = lv_label_create(footer_cont);
    lv_obj_align(alert_count_label, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_text_font(alert_count_label, &lv_font_montserrat_14, 0);

    lv_obj_move_foreground(footer_cont);

    return ESP_OK;
}

void ui_footer_update(void)
{
    if (!footer_cont) return;

    const char *status_str = "NORMAL";
    lv_color_t status_color = lv_color_make(0, 180, 0);
    lv_color_t bg_color = lv_color_make(10, 10, 30);

    switch (g_gs.system_state) {
        case SYSTEM_STATE_NORMAL:
            status_str = "NORMAL";
            status_color = lv_color_make(0, 200, 0);
            bg_color = lv_color_make(0, 20, 0);
            break;
        case SYSTEM_STATE_DEGRADED:
            status_str = "ALERTA";
            status_color = lv_color_make(200, 180, 0);
            bg_color = lv_color_make(30, 25, 0);
            break;
        case SYSTEM_STATE_SAFE_OFF:
            status_str = "SAFE_OFF";
            status_color = lv_color_make(255, 60, 0);
            bg_color = lv_color_make(40, 0, 0);
            break;
        case SYSTEM_STATE_EMERGENCY:
            status_str = "EMERG.";
            status_color = lv_color_make(255, 0, 0);
            bg_color = lv_color_make(50, 0, 0);
            break;
    }

    lv_obj_set_style_bg_color(footer_cont, bg_color, 0);
    lv_label_set_text(status_label, status_str);
    lv_obj_set_style_text_color(status_label, status_color, 0);

    uint64_t uptime = g_gs.uptime_s;
    uint64_t h = uptime / SECS_PER_HOUR;
    uint64_t m = (uptime % SECS_PER_HOUR) / SECS_PER_MINUTE;
    uint64_t s = uptime % SECS_PER_MINUTE;
    char uptime_buf[32];
    snprintf(uptime_buf, sizeof(uptime_buf), "%lluh %02llum %02llus",
             (unsigned long long)h, (unsigned long long)m, (unsigned long long)s);
    lv_label_set_text(uptime_label, uptime_buf);

    char alert_buf[24];
    snprintf(alert_buf, sizeof(alert_buf), LV_SYMBOL_BELL " %d", g_gs.active_alerts_count);
    lv_label_set_text(alert_count_label, alert_buf);
    lv_obj_set_style_text_color(alert_count_label,
        g_gs.active_alerts_count > 0 ? lv_color_make(255, 60, 60) : lv_color_make(150, 150, 150), 0);

    lv_obj_move_foreground(footer_cont);
}
