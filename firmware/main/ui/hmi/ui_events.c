// @requirement RF-UI-INPUT-001 Sistema de eventos desacoplados para ações de usuário
// @requirement RF-UI-MUTE-001 Evento MUTE
// @requirement RF-FEED-001 Solicitação de Feed Mode (confirmada pela tela)
// @requirement RF-ALERT-004 ACK de alertas via UI
#include "ui_events.h"

#include "esp_timer.h"

#include "command_validator.h"
#include "global_state.h"
#include "system_types.h"
#include "services/alert_manager.h"
#include "services/safe_state_ack.h"
#include "services/audit_log.h"
#include "driver_buzzer_led.h"

extern global_state_t g_gs;
extern bool g_feed_request;

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
            /* @requirement RF-ALERT-004 ACK em massa: alertas CRÍTICOS exigem duplo-ACK
             * (dois eventos), os demais são confirmados num único ACK. */
            cmd_validation_t cv = command_validator_can_ack_alert(&g_gs, 0);
            if (!cv.allowed) return;
            uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
            for (int16_t id = 1; id <= 65; id++) {
                if (!alert_manager_is_active(id)) continue;
                if (alert_manager_ext_double_ack_required(id)) {
                    alert_manager_ext_ack_critical(id, now_s);
                } else {
                    alert_manager_ack(id, now_s);
                }
                safe_state_ack_on_alert_ack(id, now_s);
            }
            audit_log_event(AUDIT_COMMAND, "ACK alerts via UI (criticos exigem 2o ACK)");
            break;
        }

        case UI_EVENT_REQUEST_MUTE: {
            /* MUTE atua apenas sobre a camada sonora; não toca ALM/LED/FSM/relé
             * nem estado global (RF-UI-MUTE-002). */
            buzzer_set_mute(mute_duration_to_ms(s_mute_duration));
            audit_log_event(AUDIT_COMMAND, "MUTE activated via UI");
            break;
        }

        case UI_EVENT_REQUEST_PLUG_ACTION:
            if (critical) return;
            if (g_gs.monitor_only_mode) return;
            /* A atuação efetiva é feita pela tela via plug_manager (rota única). */
            break;

        default:
            break;
    }
}
