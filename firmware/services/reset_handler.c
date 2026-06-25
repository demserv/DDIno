// @requirement RF-RESET-001 Hard reset seguro
// @requirement RF-RESET-002 Dupla confirmacao
// @requirement RF-RESET-003 Countdown com abort
// @requirement RF-RESET-004 Reset via API

#include "services/reset_handler.h"
#include "services/audit_log.h"
#include "services/storage_sd.h"
#include "hardware_config.h"
#include "global_state.h"
#include "drivers/driver_relay.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "reset_handler";

typedef struct {
    reset_state_t state;
    uint64_t deadline_ms;
} reset_handle_t;

static reset_handle_t s_reset = { .state = RESET_STATE_IDLE, .deadline_ms = 0 };

static const char *state_str(reset_state_t s)
{
    switch (s) {
        case RESET_STATE_IDLE:      return "idle";
        case RESET_STATE_CONFIRM1:  return "confirm1";
        case RESET_STATE_CONFIRM2:  return "confirm2";
        case RESET_STATE_COUNTDOWN: return "countdown";
        case RESET_STATE_ERASING:   return "erasing";
        case RESET_STATE_REBOOTING: return "rebooting";
        default:                    return "unknown";
    }
}

esp_err_t reset_handler_init(void)
{
    s_reset.state = RESET_STATE_IDLE;
    s_reset.deadline_ms = 0;
    ESP_LOGI(TAG, "Factory reset handler initialized");
    return ESP_OK;
}

esp_err_t reset_handler_start(void)
{
    if (s_reset.state == RESET_STATE_IDLE) {
        uint64_t now = (uint64_t)(esp_timer_get_time() / 1000ULL);
        s_reset.state = RESET_STATE_CONFIRM1;
        s_reset.deadline_ms = now + (uint64_t)HW_RESTART_CONFIRM_WINDOW_S * 1000ULL;
        ESP_LOGW(TAG, "Factory reset initiated - confirmation required within %ds",
                 HW_RESTART_CONFIRM_WINDOW_S);
        audit_log_event(AUDIT_FACTORY_RESET, "Factory reset initiated - CONFIRM1");
        return ESP_OK;
    }
    if (s_reset.state == RESET_STATE_CONFIRM1) {
        return reset_handler_confirm();
    }
    ESP_LOGE(TAG, "Cannot start reset in state %s", state_str(s_reset.state));
    return ESP_ERR_INVALID_STATE;
}

esp_err_t reset_handler_confirm(void)
{
    if (s_reset.state != RESET_STATE_CONFIRM1) {
        ESP_LOGE(TAG, "Cannot confirm in state %s", state_str(s_reset.state));
        return ESP_ERR_INVALID_STATE;
    }
    uint64_t now = (uint64_t)(esp_timer_get_time() / 1000ULL);
    s_reset.state = RESET_STATE_CONFIRM2;
    s_reset.deadline_ms = now + (uint64_t)HW_RESTART_CONFIRM_WINDOW_S * 1000ULL;
    ESP_LOGW(TAG, "Factory reset confirmed");
    audit_log_event(AUDIT_FACTORY_RESET, "Factory reset confirmed - CONFIRM2");
    return ESP_OK;
}

esp_err_t reset_handler_abort(void)
{
    if (s_reset.state == RESET_STATE_IDLE) {
        return ESP_ERR_INVALID_STATE;
    }
    reset_state_t prev = s_reset.state;
    s_reset.state = RESET_STATE_IDLE;
    s_reset.deadline_ms = 0;
    ESP_LOGW(TAG, "Factory reset aborted from %s", state_str(prev));
    audit_log_event(AUDIT_FACTORY_RESET, "Factory reset aborted");
    return ESP_OK;
}

void reset_handler_tick(uint64_t now_ms)
{
    switch (s_reset.state) {
        case RESET_STATE_IDLE:
            break;

        case RESET_STATE_CONFIRM1:
            if (now_ms >= s_reset.deadline_ms) {
                ESP_LOGW(TAG, "Factory reset confirmation window expired");
                s_reset.state = RESET_STATE_IDLE;
                s_reset.deadline_ms = 0;
                audit_log_event(AUDIT_FACTORY_RESET, "Factory reset auto-aborted - window expired");
            }
            break;

        case RESET_STATE_CONFIRM2:
            s_reset.state = RESET_STATE_COUNTDOWN;
            s_reset.deadline_ms = now_ms + (uint64_t)HW_RESTART_COUNTDOWN_DEFAULT_S * 1000ULL;
            ESP_LOGW(TAG, "Factory reset countdown %ds", HW_RESTART_COUNTDOWN_DEFAULT_S);
            break;

        case RESET_STATE_COUNTDOWN:
            if (now_ms >= s_reset.deadline_ms) {
                ESP_LOGW(TAG, "Factory reset countdown finished - erasing NVS");
                audit_log_event(AUDIT_FACTORY_RESET, "Factory reset countdown complete - erasing");
                s_reset.state = RESET_STATE_ERASING;
            }
            break;

        case RESET_STATE_ERASING:
            relay_all_off();
            nvs_flash_erase();
            nvs_flash_init();
            storage_sd_unmount();
            audit_log_event(AUDIT_FACTORY_RESET, "Factory reset complete - rebooting");
            ESP_LOGW(TAG, "Factory reset complete - restarting");
            s_reset.state = RESET_STATE_REBOOTING;
            esp_restart();
            break;

        case RESET_STATE_REBOOTING:
            break;
    }
}

bool reset_handler_is_pending(void)
{
    return s_reset.state != RESET_STATE_IDLE;
}

int reset_handler_remaining_s(void)
{
    if (s_reset.state == RESET_STATE_IDLE || s_reset.deadline_ms == 0) return 0;
    uint64_t now = (uint64_t)(esp_timer_get_time() / 1000ULL);
    if (now >= s_reset.deadline_ms) return 0;
    return (int)((s_reset.deadline_ms - now) / 1000ULL);
}

reset_state_t reset_handler_get_state(void)
{
    return s_reset.state;
}
