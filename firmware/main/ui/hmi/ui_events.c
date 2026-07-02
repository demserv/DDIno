// @requirement RF-UI-INPUT-001 Sistema de eventos desacoplados para ações de usuário
// @requirement RF-UI-MUTE-001 Evento MUTE
// @requirement RF-FEED-001 Solicitação de Feed Mode (confirmada pela tela)
// @requirement RF-ALERT-004 ACK de alertas via UI
#include "ui_events.h"

#include "esp_timer.h"

#include "command_validator.h"
#include "global_state.h"
#include "system_types.h"
#include "alert_manager.h"
#include "maintenance_mode.h"
#include "safe_state_ack.h"
#include "audit_log.h"
#include "driver_buzzer_led.h"
#include "reset_handler.h"
#include "ui_app.h"
#include "ui_screen_manager.h"
#include "screens/ui_screen_config_temperature.h"
#include "profile_manager.h"
#include "plug_manager.h"
#include "esp_system.h"
#include "components/ui_confirm_dialog.h"
#include "components/ui_inline_hint.h"

void app_ato_clear_blocked(void);

extern global_state_t g_gs;
extern bool g_feed_request;
extern bool g_feed_exit_request;

static ui_confirm_dialog_t s_plug_confirm;
static ui_inline_hint_t s_plug_hint;
static bool s_plug_confirm_ready = false;
static uint8_t s_pending_plug_id = 0;
static bool s_pending_plug_on = false;
static uint8_t s_plug_action_id = 0;

static void plug_confirm_ok_cb(lv_event_t *e);
static void plug_confirm_cancel_cb(lv_event_t *e);

static void plug_confirm_init(void)
{
    if (s_plug_confirm_ready) return;
    ui_confirm_dialog_create(&s_plug_confirm, lv_layer_top(),
                             "Confirmar acao em plugue critico P01/P02?");
    ui_confirm_dialog_set_confirm_cb(&s_plug_confirm, plug_confirm_ok_cb);
    ui_confirm_dialog_set_cancel_cb(&s_plug_confirm, plug_confirm_cancel_cb);
    ui_inline_hint_create(&s_plug_hint, lv_layer_top());
    s_plug_confirm_ready = true;
}

static void plug_toggle_apply(uint8_t plug_id, bool want_on)
{
    esp_err_t err = plug_manager_toggle((plug_id_t)plug_id, want_on);
    if (err == ESP_OK) {
        char msg[48];
        snprintf(msg, sizeof(msg), "Plug P%02u toggled %s", (unsigned)plug_id, want_on ? "ON" : "OFF");
        audit_log_event(AUDIT_COMMAND, msg);
        ui_app_refresh_now();
    }
}

static void plug_confirm_ok_cb(lv_event_t *e)
{
    (void)e;
    ui_confirm_dialog_show(&s_plug_confirm, false);
    if (s_pending_plug_id >= 1 && s_pending_plug_id <= 10) {
        plug_toggle_apply(s_pending_plug_id, s_pending_plug_on);
    }
    s_pending_plug_id = 0;
}

static void plug_confirm_cancel_cb(lv_event_t *e)
{
    (void)e;
    ui_confirm_dialog_show(&s_plug_confirm, false);
    s_pending_plug_id = 0;
}

static void plug_toggle_denied_hint(const char *reason)
{
    plug_confirm_init();
    if (reason && reason[0]) {
        ui_inline_hint_show(&s_plug_hint, reason, 2500);
    }
}

/* @requirement RF-UI-MUTE-001 Duração do MUTE selecionável (5/10/15 min ou até-ACK).
 * "Até-ACK" é representado por uma duração muito longa (limpa no ACK do alerta). */
#define UI_MUTE_UNTIL_ACK_MS (24U * 60U * 60U * 1000U)
static ui_mute_duration_t s_mute_duration = UI_MUTE_DURATION_5MIN;

static uint32_t mute_duration_to_ms(ui_mute_duration_t d)
{
    switch (d) {
        case UI_MUTE_DURATION_10MIN:     return 10U * 60U * 1000U;
        case UI_MUTE_DURATION_15MIN:     return 15U * 60U * 1000U;
        case UI_MUTE_DURATION_UNTIL_ACK: return UI_MUTE_UNTIL_ACK_MS;
        case UI_MUTE_DURATION_5MIN:
        default:                         return 5U * 60U * 1000U;
    }
}

void ui_events_set_mute_duration(ui_mute_duration_t duration)
{
    s_mute_duration = duration;
    audit_log_event(AUDIT_COMMAND, "MUTE duration changed via UI");
}

ui_mute_duration_t ui_events_get_mute_duration(void)
{
    return s_mute_duration;
}

void ui_events_ack_alert(int16_t alm_id)
{
    if (alm_id <= 0) return;
    cmd_validation_t cv = command_validator_can_ack_alert(&g_gs, (uint16_t)alm_id);
    if (!cv.allowed) return;
    if (!alert_manager_is_active(alm_id)) return;
    uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
    alert_manager_ack_with_policy(alm_id, now_s);
    safe_state_ack_on_alert_ack(alm_id, now_s);
    audit_log_event(AUDIT_COMMAND, "ACK alert via UI (single)");
    ui_app_refresh_now();
}

void ui_events_emit(ui_event_t event)
{
    /* EMERGENCY/SAFE_OFF: bloqueia ações que não sejam ACK (RF-UI-OVERLAY-001).
     * ACK e MUTE permanecem disponíveis pois não religam cargas nem ocultam o
     * estado crítico. */
    bool critical = (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF);

    switch (event) {
        case UI_EVENT_REQUEST_FEED_MODE: {
            if (critical) return;
            cmd_validation_t cv = command_validator_can_start_feed(&g_gs);
            if (!cv.allowed) return;
            g_feed_request = true;
            audit_log_event(AUDIT_FEED_MODE, "Feed mode requested via UI");
            break;
        }

        case UI_EVENT_REQUEST_ACK_ALERT: {
            /* @requirement RF-ALERT-004 ACK em massa por instância ativa. */
            cmd_validation_t cv = command_validator_can_ack_alert(&g_gs, 0);
            if (!cv.allowed) return;
            uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
            alert_slot_t slots[ALERT_SLOTS_MAX];
            uint16_t count = 0;
            alert_manager_get_active_slots(slots, &count, ALERT_SLOTS_MAX);
            for (uint16_t i = 0; i < count; i++) {
                alert_manager_ack_with_policy_instance(slots[i].alm_id, slots[i].related_plug_id, now_s);
                safe_state_ack_on_alert_ack(slots[i].alm_id, now_s);
            }
            audit_log_event(AUDIT_COMMAND, "ACK alerts via UI (criticos exigem 2o ACK)");
            ui_app_refresh_now();
            break;
        }

        case UI_EVENT_REQUEST_MUTE: {
            buzzer_set_mute(mute_duration_to_ms(s_mute_duration));
            {
                uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
                uint64_t until = now_s + (uint64_t)(mute_duration_to_ms(s_mute_duration) / 1000U);
                alert_manager_set_silenced_all_active(until);
            }
            audit_log_event(AUDIT_COMMAND, "MUTE activated via UI");
            break;
        }

        case UI_EVENT_REQUEST_SAVE_THERMAL_CONFIG:
            if (ui_screen_config_temperature_save()) {
                audit_log_event(AUDIT_CONFIG_CHANGE, "Thermal config saved via UI");
            }
            ui_app_refresh_now();
            break;

        case UI_EVENT_REQUEST_CONFIG_EXPORT:
            profile_manager_save("backup");
            audit_log_event(AUDIT_COMMAND, "Config export via UI (profile backup)");
            break;

        case UI_EVENT_REQUEST_CONFIG_IMPORT:
            profile_manager_load("backup");
            audit_log_event(AUDIT_COMMAND, "Config import via UI (profile backup)");
            ui_app_refresh_now();
            break;

        case UI_EVENT_REQUEST_PLUG_ACTION:
            if (critical) return;
            if (g_gs.monitor_only_mode) return;
            if (s_plug_action_id >= 1 && s_plug_action_id <= 10) {
                ui_events_toggle_plug(s_plug_action_id);
                s_plug_action_id = 0;
            }
            break;

        case UI_EVENT_REQUEST_FACTORY_RESET: {
            /* @requirement RF-UI-MENU-001 / RF-RESET-002 Caminho de menu para reset de
             * fábrica com dupla confirmação: cada emissão avança a FSM segura
             * (IDLE→CONFIRM1→CONFIRM2/COUNTDOWN). O countdown permite abort. */
            reset_state_t st = reset_handler_get_state();
            if (st == RESET_STATE_CONFIRM1) {
                reset_handler_confirm();
                audit_log_event(AUDIT_COMMAND, "Factory reset confirmado via UI");
            } else if (st == RESET_STATE_IDLE) {
                reset_handler_start();
                audit_log_event(AUDIT_COMMAND, "Factory reset iniciado via UI (confirme novamente)");
            }
            ui_app_refresh_now();
            break;
        }

        case UI_EVENT_REQUEST_SAFE_REBOOT:
            audit_log_event(AUDIT_COMMAND, "Reboot controlado via UI");
            esp_restart();
            break;

        case UI_EVENT_NAVIGATE_HOME:
            ui_screen_manager_show(UI_SCREEN_DASHBOARD);
            ui_app_refresh_now();
            break;

        case UI_EVENT_NAVIGATE_BACK:
            ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
            ui_app_refresh_now();
            break;

        case UI_EVENT_REQUEST_EXIT_FEED_MODE:
            g_feed_exit_request = true;
            audit_log_event(AUDIT_FEED_MODE, "Feed exit requested via UI");
            ui_app_refresh_now();
            break;

        default:
            break;
    }
}

void ui_events_unblock_plug(uint8_t plug_id)
{
    if (plug_id < 1 || plug_id > 10) return;
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) return;
    if (g_gs.monitor_only_mode) return;
    if (!maintenance_mode_is_active()) return;

    esp_err_t err = plug_manager_unblock((plug_id_t)plug_id);
    if (err == ESP_OK) {
        char msg[48];
        snprintf(msg, sizeof(msg), "Plug P%02u desbloqueado", (unsigned)plug_id);
        audit_log_event(AUDIT_COMMAND, msg);
        ui_app_refresh_now();
    }
}

void ui_events_set_plug_action_target(uint8_t plug_id)
{
    s_plug_action_id = plug_id;
}

void ui_events_request_plug_action(uint8_t plug_id)
{
    ui_events_set_plug_action_target(plug_id);
    ui_events_emit(UI_EVENT_REQUEST_PLUG_ACTION);
}

void ui_events_toggle_plug(uint8_t plug_id)
{
    if (plug_id < 1 || plug_id > 10) return;
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) return;

    plug_model_t *pm = plug_manager_get((plug_id_t)plug_id);
    if (!pm) return;
    bool want_on = (pm->effective_state != PLUG_EFFECTIVE_STATE_ON);

    cmd_validation_t cv = command_validator_can_toggle_plug(&g_gs, plug_id, want_on);
    if (!cv.allowed) {
        plug_toggle_denied_hint(cv.error_code ? cv.error_code : "Acao negada");
        return;
    }

    if (cv.requires_double_confirmation) {
        plug_confirm_init();
        s_pending_plug_id = plug_id;
        s_pending_plug_on = want_on;
        char msg[64];
        snprintf(msg, sizeof(msg), "Confirmar %s de P%02u?", want_on ? "ligar" : "desligar", (unsigned)plug_id);
        lv_label_set_text(s_plug_confirm.message_label, msg);
        ui_confirm_dialog_show(&s_plug_confirm, true);
        return;
    }

    plug_toggle_apply(plug_id, want_on);
}

void ui_events_clear_ato_blocked(void)
{
    if (!maintenance_mode_is_active()) return;
    app_ato_clear_blocked();
    audit_log_event(AUDIT_COMMAND, "ATO blocked cleared via UI");
    ui_app_refresh_now();
}
