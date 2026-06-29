// @requirement RF-UI-STATUS-001 Barra de status persistente (header grafico)
// @requirement RF-UI-OVERLAY-001 Overlay critico com Feed toggle
#include "ui_header.h"

#include "esp_log.h"

#include "driver_ds3231.h"
#include "global_state.h"

static const char *TAG = "ui_header";

static lv_obj_t *header_cont = NULL;
static lv_obj_t *wifi_icon = NULL;
static lv_obj_t *clock_label = NULL;
static lv_obj_t *feed_btn = NULL;

extern global_state_t g_gs;

static void feed_btn_cb(lv_event_t *e)
{
    extern bool g_feed_request;
    if (g_gs.system_state < SYSTEM_STATE_SAFE_OFF && !g_gs.feed_active) {
        g_feed_request = true;
    }
}

esp_err_t ui_header_init(void)
{
    ESP_LOGI(TAG, "Initializing graphical header");

    header_cont = lv_obj_create(lv_layer_top());
    lv_obj_set_size(header_cont, 480, 40);
    lv_obj_set_style_radius(header_cont, 0, 0);
    lv_obj_set_style_border_width(header_cont, 0, 0);
    lv_obj_set_style_pad_all(header_cont, 0, 0);
    lv_obj_clear_flag(header_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(header_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header_cont, lv_color_make(10, 10, 30), 0);

    wifi_icon = lv_label_create(header_cont);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI " ---");
    lv_obj_align(wifi_icon, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_14, 0);

    clock_label = lv_label_create(header_cont);
    lv_label_set_text(clock_label, "--/--/---- --:--");
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(clock_label, lv_color_make(200, 200, 255), 0);

    feed_btn = lv_btn_create(header_cont);
    lv_obj_set_size(feed_btn, 56, 28);
    lv_obj_align(feed_btn, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(feed_btn, lv_color_make(180, 140, 0), 0);
    lv_obj_set_style_bg_color(feed_btn, lv_color_make(80, 60, 0), LV_STATE_PRESSED);
    lv_obj_add_event_cb(feed_btn, feed_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *feed_lbl = lv_label_create(feed_btn);
    lv_label_set_text(feed_lbl, "FEED");
    lv_obj_center(feed_lbl);

    lv_obj_move_foreground(header_cont);

    return ESP_OK;
}

void ui_header_update(void)
{
    if (!header_cont) return;

    char wifi_buf[24];
    snprintf(wifi_buf, sizeof(wifi_buf), "%s %s",
             LV_SYMBOL_WIFI, g_gs.wifi_ok ? "ON" : "OFF");
    lv_label_set_text(wifi_icon, wifi_buf);
    lv_obj_set_style_text_color(wifi_icon,
        g_gs.wifi_ok ? lv_color_make(0, 200, 0) : lv_color_make(150, 150, 150), 0);

    if (g_gs.time_valid) {
        ds3231_time_t rt;
        if (ds3231_get_time(&rt) == ESP_OK) {
            char time_buf[24];
            snprintf(time_buf, sizeof(time_buf), "%02u/%02u/%04u %02u:%02u",
                     (unsigned)rt.day, (unsigned)rt.month, (unsigned)rt.year,
                     (unsigned)rt.hour, (unsigned)rt.minute);
            lv_label_set_text(clock_label, time_buf);
        }
    } else {
        lv_label_set_text(clock_label, "--/--/---- --:--");
    }

    if (g_gs.feed_active) {
        lv_obj_set_style_bg_color(feed_btn, lv_color_make(200, 100, 0), 0);
        lv_obj_add_state(feed_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_set_style_bg_color(feed_btn, lv_color_make(180, 140, 0), 0);
        lv_obj_clear_state(feed_btn, LV_STATE_DISABLED);
    }

    lv_obj_move_foreground(header_cont);
}

