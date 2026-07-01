#include "command_dispatcher.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "alert_manager.h"
#include "command_validator.h"
#include "plug_manager.h"
#include "safety_gate.h"

static const char *TAG = "cmd_dispatch";

/* Ponte de pedido de Feed consumida por app_main (rota única de ativação). */
extern bool g_feed_request;

esp_err_t command_dispatch_execute(const command_entry_t *entry, const global_state_t *gs)
{
    if (!entry || !gs) return ESP_ERR_INVALID_ARG;

    safety_gate_result_t gate = safety_gate_can_enable_automation(gs);
    if (!gate.can_automate) {
        ESP_LOGW(TAG, "Blocked by safety gate: %s", gate.block_reason);
        return ESP_ERR_INVALID_STATE;
    }

    if (entry->handler) {
        return entry->handler(entry->ctx);
    }

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t command_dispatch_toggle_plug(const global_state_t *gs, uint8_t plug_id, bool desired_on)
{
    if (!gs) return ESP_ERR_INVALID_ARG;

    cmd_validation_t v = command_validator_can_toggle_plug(gs, plug_id, desired_on);
    if (!v.allowed) {
        ESP_LOGW(TAG, "Plug toggle denied: %s", v.error_code);
        return ESP_ERR_INVALID_STATE;
    }

    return plug_manager_toggle((plug_id_t)plug_id, desired_on);
}

esp_err_t command_dispatch_ack_alert(const global_state_t *gs, uint16_t alert_id)
{
    if (!gs) return ESP_ERR_INVALID_ARG;

    cmd_validation_t v = command_validator_can_ack_alert(gs, alert_id);
    if (!v.allowed) {
        ESP_LOGW(TAG, "Alert ack denied: %s", v.error_code);
        return ESP_ERR_INVALID_STATE;
    }

    uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
    return alert_manager_ack((int16_t)alert_id, now_s) ? ESP_OK : ESP_FAIL;
}

esp_err_t command_dispatch_start_feed(const global_state_t *gs)
{
    if (!gs) return ESP_ERR_INVALID_ARG;

    cmd_validation_t v = command_validator_can_start_feed(gs);
    if (!v.allowed) {
        ESP_LOGW(TAG, "Feed denied: %s", v.error_code);
        return ESP_ERR_INVALID_STATE;
    }

    /* Ativação real do Feed é feita por app_main ao consumir g_feed_request
     * (mesma rota usada por api_rest e ui_events). */
    g_feed_request = true;
    return ESP_OK;
}

esp_err_t command_dispatch_set_mode(const global_state_t *gs, uint8_t plug_id, plug_mode_t mode)
{
    if (!gs) return ESP_ERR_INVALID_ARG;

    cmd_validation_t v = command_validator_can_set_mode(gs, plug_id);
    if (!v.allowed) {
        ESP_LOGW(TAG, "Set mode denied: %s", v.error_code);
        return ESP_ERR_INVALID_STATE;
    }

    return plug_manager_set_mode((plug_id_t)plug_id, mode);
}

