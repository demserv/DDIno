#include "command_dispatcher.h"
#include "command_validator.h"
#include "safety_gate.h"
#include "plug_manager.h"
#include "alert_manager.h"
#include "feed_snapshot.h"
#include "esp_log.h"

static const char *TAG = "cmd_dispatch";

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

    return plug_manager_set(plug_id, desired_on);
}

esp_err_t command_dispatch_ack_alert(const global_state_t *gs, uint16_t alert_id)
{
    if (!gs) return ESP_ERR_INVALID_ARG;

    cmd_validation_t v = command_validator_can_ack_alert(gs, alert_id);
    if (!v.allowed) {
        ESP_LOGW(TAG, "Alert ack denied: %s", v.error_code);
        return ESP_ERR_INVALID_STATE;
    }

    return alert_manager_ack(alert_id);
}

esp_err_t command_dispatch_start_feed(const global_state_t *gs)
{
    if (!gs) return ESP_ERR_INVALID_ARG;

    cmd_validation_t v = command_validator_can_start_feed(gs);
    if (!v.allowed) {
        ESP_LOGW(TAG, "Feed denied: %s", v.error_code);
        return ESP_ERR_INVALID_STATE;
    }

    return feed_snapshot_start();
}

esp_err_t command_dispatch_set_mode(const global_state_t *gs, uint8_t plug_id)
{
    if (!gs) return ESP_ERR_INVALID_ARG;

    cmd_validation_t v = command_validator_can_set_mode(gs, plug_id);
    if (!v.allowed) {
        ESP_LOGW(TAG, "Set mode denied: %s", v.error_code);
        return ESP_ERR_INVALID_STATE;
    }

    return plug_manager_set_mode(plug_id);
}
