// @requirement RF-UI-OVERLAY-001 Tela de alertas com overlays críticos
// @requirement RF-ALERT-004 Timeout de ACK visível na UI
// @requirement NC-009 ACK crítico exige confirmação em dois estágios
#include "esp_timer.h"
#include "lvgl.h"

#include "../ui_screens.h"
#include "alert_manager.h"
#include "alert_manager_ext.h"
#include "alert_model.h"
#include "global_state.h"

static lv_obj_t *count_label = NULL;
static lv_obj_t *active_list = NULL;
static lv_obj_t *history_label = NULL;
static lv_obj_t *no_alerts_label = NULL;
static lv_obj_t *ack_btn = NULL;
static lv_obj_t *critical_confirm_label = NULL;

extern global_state_t g_gs;

static void screen_update_alerts(void);

static void ack_btn_cb(lv_event_t *e)
{
    (void)e;
    uint64_t now = esp_timer_get_time() / 1000000ULL;
    bool has_critical = false;
    for (int16_t id = 1; id <= 65; id++) {
        if (!alert_manager_is_active(id)) continue;
        const alert_slot_t *a = alert_manager_get_slot(id);
        if (!a) continue;
        if (a->severity == ALERT_SEVERITY_CRITICAL) {
            has_critical = true;
            uint8_t stage = alert_manager_ext_get_ack_stage(id);
            if (stage < ACK_STAGE_CONFIRMED) {
                alert_manager_ext_ack_critical(id, now);
            }
        } else {
            alert_manager_ack(id, now);
        }
    }
    if (!has_critical) {
        for (int16_t id = 1; id <= 65; id++) {
            if (alert_manager_is_active(id)) {
                alert_manager_ack(id, now);
            }
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

    critical_confirm_label = lv_label_create(parent);
    lv_label_set_text(critical_confirm_label, "ALERTA CRITICO: Confirme novamente para ACK");
    lv_obj_set_style_text_color(critical_confirm_label, lv_color_make(255, 0, 0), 0);
    lv_obj_align(critical_confirm_label, LV_ALIGN_BOTTOM_LEFT, 10, -4);
    lv_obj_add_flag(critical_confirm_label, LV_OBJ_FLAG_HIDDEN);
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
        lv_obj_add_flag(critical_confirm_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_add_flag(no_alerts_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ack_btn, LV_OBJ_FLAG_HIDDEN);

    bool critical_pending_stage1 = false;

    for (int16_t id = 1; id <= 65; id++) {
        if (!alert_manager_is_active(id)) continue;
        const alert_slot_t *a = alert_manager_get_slot(id);
        if (!a) continue;

        lv_obj_t *item = lv_list_add_btn(active_list, NULL, "");

        const char *sev_icon = LV_SYMBOL_WARNING;
        lv_color_t sev_color = lv_color_make(200, 200, 0);
        const char *ack_status = "";
        switch (a->severity) {
            case ALERT_SEVERITY_CRITICAL: {
                sev_icon = LV_SYMBOL_WARNING;
                sev_color = lv_color_make(255, 0, 0);
                uint8_t stage = alert_manager_ext_get_ack_stage(id);
                if (stage == ACK_STAGE_NONE) {
                    ack_status = "Pend.";
                    critical_pending_stage1 = true;
                } else if (stage == ACK_STAGE_FIRST) {
                    ack_status = "Confirme novamente!";
                    critical_pending_stage1 = true;
                } else {
                    ack_status = "ACK";
                }
                break;
            }
            case ALERT_SEVERITY_HIGH:
                sev_icon = LV_SYMBOL_WARNING;
                sev_color = lv_color_make(255, 100, 0);
                ack_status = a->acked ? "ACK" : "Pend.";
                break;
            case ALERT_SEVERITY_WARNING:
                sev_icon = LV_SYMBOL_WARNING;
                sev_color = lv_color_make(200, 180, 0);
                ack_status = a->acked ? "ACK" : "Pend.";
                break;
            default:
                sev_icon = LV_SYMBOL_OK;
                sev_color = lv_color_make(180, 180, 180);
                ack_status = a->acked ? "ACK" : "Pend.";
                break;
        }

        snprintf(buf, sizeof(buf), "%s ALM-%03d: %s (%s)",
                 sev_icon, id, a->message, ack_status);
        lv_label_set_text(item, buf);
        lv_obj_set_style_text_color(item, sev_color, 0);
    }

    if (critical_pending_stage1) {
        lv_obj_clear_flag(critical_confirm_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(critical_confirm_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void __attribute__((constructor)) register_alerts(void)
{
    ui_screen_register(SCREEN_ALERTS, screen_init_alerts, screen_update_alerts);
}

