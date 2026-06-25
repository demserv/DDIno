#include "ui_screens.h"
#include "ui_state_badge.h"
#include "global_state.h"
#include "alert_manager.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui_screens";

static lv_obj_t *screens[SCREEN_COUNT];
static screen_init_fn_t init_fns[SCREEN_COUNT];
static screen_update_fn_t update_fns[SCREEN_COUNT];
static int current_idx = 0;

static esp_timer_handle_t screen_timeout_timer;
static bool timeout_active = false;

static lv_obj_t *alert_overlay = NULL;
static lv_obj_t *alert_title_label = NULL;
static lv_obj_t *alert_alm_label = NULL;
static lv_obj_t *alert_severity_label = NULL;
static lv_obj_t *alert_action_label = NULL;
static lv_obj_t *alert_ack_label = NULL;

extern global_state_t g_gs;

static void on_screen_timeout(void *arg)
{
    ui_screen_show(SCREEN_DASHBOARD);
}

static void restart_timeout(void)
{
    if (timeout_active) {
        esp_timer_stop(screen_timeout_timer);
        esp_timer_start_once(screen_timeout_timer, 60 * 1000 * 1000);
    }
}

void ui_screen_notify_activity(void)
{
    restart_timeout();
}

void ui_screen_register(screen_id_t id, screen_init_fn_t init, screen_update_fn_t update)
{
    if (id < SCREEN_COUNT) {
        init_fns[id] = init;
        update_fns[id] = update;
    }
}

esp_err_t ui_screens_init(void)
{
    ESP_LOGI(TAG, "Initializing screens");

    for (int i = 0; i < SCREEN_COUNT; i++) {
        screens[i] = lv_obj_create(NULL);
        lv_obj_set_size(screens[i], 480, 320);
    }

    for (int i = 0; i < SCREEN_COUNT; i++) {
        if (init_fns[i]) {
            init_fns[i](screens[i]);
        }
    }

    esp_timer_create_args_t timeout_args = {
        .callback = on_screen_timeout,
        .name = "screen_timeout"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timeout_args, &screen_timeout_timer));
    timeout_active = true;

    alert_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(alert_overlay, 460, 300);
    lv_obj_center(alert_overlay);
    lv_obj_set_style_bg_color(alert_overlay, lv_color_make(30, 0, 0), 0);
    lv_obj_set_style_border_color(alert_overlay, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_width(alert_overlay, 3, 0);
    lv_obj_set_style_radius(alert_overlay, 8, 0);
    lv_obj_set_style_pad_all(alert_overlay, 10, 0);
    lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);

    alert_title_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_title_label, "ALERTA CRITICO");
    lv_obj_set_style_text_color(alert_title_label, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_text_font(alert_title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(alert_title_label, LV_ALIGN_TOP_MID, 0, 10);

    alert_alm_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_alm_label, "ALM-000");
    lv_obj_set_style_text_color(alert_alm_label, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_text_font(alert_alm_label, &lv_font_montserrat_16, 0);
    lv_obj_align(alert_alm_label, LV_ALIGN_TOP_LEFT, 10, 45);

    alert_severity_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_severity_label, "Severidade: CRITICAL");
    lv_obj_set_style_text_color(alert_severity_label, lv_color_make(255, 200, 0), 0);
    lv_obj_align(alert_severity_label, LV_ALIGN_TOP_LEFT, 10, 70);

    alert_action_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_action_label, "Acao: Desligar cargas e verificar equipamento");
    lv_obj_set_style_text_color(alert_action_label, lv_color_make(255, 255, 255), 0);
    lv_obj_align(alert_action_label, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_obj_set_size(alert_action_label, 430, 120);
    lv_label_set_long_mode(alert_action_label, LV_LABEL_LONG_WRAP);

    alert_ack_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_ack_label, "ACK: Enter ou toque para reconhecer");
    lv_obj_set_style_text_color(alert_ack_label, lv_color_make(200, 200, 200), 0);
    lv_obj_align(alert_ack_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_scr_load(screens[SCREEN_DASHBOARD]);
    restart_timeout();

    ESP_LOGI(TAG, "Screens initialized");
    return ESP_OK;
}

void ui_screen_show(screen_id_t id)
{
    if (id < SCREEN_COUNT && screens[id]) {
        current_idx = id;
        lv_scr_load(screens[id]);
        if (update_fns[id]) {
            update_fns[id]();
        }
        restart_timeout();
    }
}

screen_id_t ui_screen_current(void)
{
    return (screen_id_t)current_idx;
}

void ui_screen_next(void)
{
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        ui_screen_show(SCREEN_ALERTS);
        return;
    }
    current_idx = (current_idx + 1) % SCREEN_COUNT;
    lv_scr_load(screens[current_idx]);
    if (update_fns[current_idx]) {
        update_fns[current_idx]();
    }
    restart_timeout();
}

void ui_screen_prev(void)
{
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        ui_screen_show(SCREEN_ALERTS);
        return;
    }
    current_idx = (current_idx - 1 + SCREEN_COUNT) % SCREEN_COUNT;
    lv_scr_load(screens[current_idx]);
    if (update_fns[current_idx]) {
        update_fns[current_idx]();
    }
    restart_timeout();
}

void ui_screen_update_all(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++) {
        if (update_fns[i]) {
            update_fns[i]();
        }
    }

    ui_state_badge_update();

    if (g_gs.critical_alerts_count > 0) {
        lv_obj_clear_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(alert_overlay);

        char buf[64];
        snprintf(buf, sizeof(buf), "ALM-XXX | %d alerta(s) critico(s)", g_gs.critical_alerts_count);
        lv_label_set_text(alert_alm_label, buf);

        const char *action = "Desligar cargas e verificar equipamento";
        if (g_gs.safeoff_reason == SAFEOFF_REASON_THERMAL_CRITICAL) {
            action = "Temperatura critica! Verificar aquecedor e cooler";
        } else if (g_gs.safeoff_reason == SAFEOFF_REASON_ATO_OVERFLOW) {
            action = "Nivel ATO em overflow! Verificar reservatorio";
        } else if (g_gs.safeoff_reason == SAFEOFF_REASON_ELECTRIC_TOTAL) {
            action = "Sobrecarga eletrica! Reduzir consumo";
        } else if (g_gs.safeoff_reason == SAFEOFF_REASON_PLUG_SHORT) {
            action = "Curto-circuito detectado! Desconectar equipamento";
        }
        lv_label_set_text(alert_action_label, action);
    } else {
        lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}
