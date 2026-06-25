#include "../ui_screens.h"
#include "global_state.h"
#include "alert_model.h"
#include "alert_manager.h"

#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "screen_alerts";

static lv_obj_t *count_label = NULL;
static lv_obj_t *alert_list = NULL;
static lv_obj_t *no_alerts_label = NULL;
static lv_obj_t *ack_btn = NULL;

extern global_state_t g_gs;

static void screen_update_alerts(void);

static void ack_btn_cb(lv_event_t *e)
{
    (void)e;
    uint64_t now = esp_timer_get_time() / 1000000ULL;
    for (int16_t id = 1; id <= 65; id++) {
        if (alert_manager_is_active(id)) {
            alert_manager_ack(id, now);
        }
    }
    screen_update_alerts();
}

static void screen_init_alerts(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Alarmes");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    count_label = lv_label_create(parent);
    lv_label_set_text(count_label, "Alarmes Ativos: 0");
    lv_obj_align(count_label, LV_ALIGN_TOP_LEFT, 10, 40);

    no_alerts_label = lv_label_create(parent);
    lv_label_set_text(no_alerts_label, "Nenhum alerta ativo");
    lv_obj_set_style_text_font(no_alerts_label, &lv_font_montserrat_16, 0);
    lv_obj_align(no_alerts_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(no_alerts_label, LV_OBJ_FLAG_HIDDEN);

    alert_list = lv_list_create(parent);
    lv_obj_set_size(alert_list, 460, 200);
    lv_obj_align(alert_list, LV_ALIGN_TOP_LEFT, 10, 70);

    ack_btn = lv_btn_create(parent);
    lv_obj_set_size(ack_btn, 120, 35);
    lv_obj_align(ack_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_add_event_cb(ack_btn, ack_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ack_lbl = lv_label_create(ack_btn);
    lv_label_set_text(ack_lbl, "ACK Todos");
    lv_obj_center(ack_lbl);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< >   Navegar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_alerts(void)
{
    uint16_t active_count = alert_manager_active_count();
    char buf[48];
    snprintf(buf, sizeof(buf), "Alarmes Ativos: %d", active_count);
    lv_label_set_text(count_label, buf);

    lv_obj_clean(alert_list);

    if (active_count == 0) {
        lv_obj_clear_flag(no_alerts_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ack_btn, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_add_flag(no_alerts_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ack_btn, LV_OBJ_FLAG_HIDDEN);
}

static void __attribute__((constructor)) register_alerts(void)
{
    ui_screen_register(SCREEN_ALERTS, screen_init_alerts, screen_update_alerts);
}
