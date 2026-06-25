#include "../ui_screens.h"
#include "global_state.h"
#include "alert_model.h"
#include "alert_manager.h"
#include "lvgl.h"
#include "esp_timer.h"

static lv_obj_t *count_label = NULL;
static lv_obj_t *active_list = NULL;
static lv_obj_t *history_label = NULL;
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
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Alertas");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    count_label = lv_label_create(parent);
    lv_label_set_text(count_label, "Ativos: 0");
    lv_obj_align(count_label, LV_ALIGN_TOP_LEFT, 10, 68);
    lv_obj_set_style_text_font(count_label, &lv_font_montserrat_14, 0);

    no_alerts_label = lv_label_create(parent);
    lv_label_set_text(no_alerts_label, "Nenhum alerta ativo");
    lv_obj_set_style_text_font(no_alerts_label, &lv_font_montserrat_16, 0);
    lv_obj_align(no_alerts_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(no_alerts_label, LV_OBJ_FLAG_HIDDEN);

    active_list = lv_list_create(parent);
    lv_obj_set_size(active_list, 460, 160);
    lv_obj_align(active_list, LV_ALIGN_TOP_LEFT, 10, 96);

    history_label = lv_label_create(parent);
    lv_label_set_text(history_label, "Historico: 0 resolvidos");
    lv_obj_align(history_label, LV_ALIGN_BOTTOM_LEFT, 10, -44);
    lv_obj_set_style_text_color(history_label, lv_color_make(150, 150, 150), 0);

    ack_btn = lv_btn_create(parent);
    lv_obj_set_size(ack_btn, 100, 30);
    lv_obj_align(ack_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -44);
    lv_obj_add_event_cb(ack_btn, ack_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ack_lbl = lv_label_create(ack_btn);
    lv_label_set_text(ack_lbl, "ACK Todos");
    lv_obj_center(ack_lbl);
}

static void screen_update_alerts(void)
{
    uint16_t active_count = alert_manager_active_count();
    char buf[192];

    snprintf(buf, sizeof(buf), "Ativos: %d", active_count);
    lv_label_set_text(count_label, buf);
    lv_obj_set_style_text_color(count_label,
        active_count > 0 ? lv_color_make(255, 60, 60) : lv_color_make(0, 200, 0), 0);

    lv_obj_clean(active_list);

    if (active_count == 0) {
        lv_obj_clear_flag(no_alerts_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ack_btn, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_add_flag(no_alerts_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ack_btn, LV_OBJ_FLAG_HIDDEN);

    for (int16_t id = 1; id <= 65; id++) {
        if (!alert_manager_is_active(id)) continue;
        const alert_slot_t *a = alert_manager_get_slot(id);
        if (!a) continue;

        lv_obj_t *item = lv_list_add_btn(active_list, NULL, "");

        const char *sev_icon = LV_SYMBOL_WARNING;
        lv_color_t sev_color = lv_color_make(200, 200, 0);
        switch (a->severity) {
            case ALERT_SEVERITY_CRITICAL:
                sev_icon = LV_SYMBOL_WARNING;
                sev_color = lv_color_make(255, 0, 0);
                break;
            case ALERT_SEVERITY_HIGH:
                sev_icon = LV_SYMBOL_WARNING;
                sev_color = lv_color_make(255, 100, 0);
                break;
            case ALERT_SEVERITY_WARNING:
                sev_icon = LV_SYMBOL_WARNING;
                sev_color = lv_color_make(200, 180, 0);
                break;
            default:
                sev_icon = LV_SYMBOL_OK;
                sev_color = lv_color_make(180, 180, 180);
                break;
        }

        snprintf(buf, sizeof(buf), "%s ALM-%03d: %s (%s)",
                 sev_icon, id, a->message, a->acked ? "ACK" : "Pend.");
        lv_label_set_text(item, buf);
        lv_obj_set_style_text_color(item, sev_color, 0);
    }
}

static void __attribute__((constructor)) register_alerts(void)
{
    ui_screen_register(SCREEN_ALERTS, screen_init_alerts, screen_update_alerts);
}
