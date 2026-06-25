#ifndef FIRMWARE_SERVICES_RESET_HANDLER_H
#define FIRMWARE_SERVICES_RESET_HANDLER_H

// @requirement RF-RESET-001 Hard reset seguro
// @requirement RF-RESET-002 Dupla confirmacao
// @requirement RF-RESET-003 Countdown com abort
// @requirement RF-RESET-004 Reset via API

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RESET_STATE_IDLE = 0,
    RESET_STATE_CONFIRM1,
    RESET_STATE_CONFIRM2,
    RESET_STATE_COUNTDOWN,
    RESET_STATE_ERASING,
    RESET_STATE_REBOOTING
} reset_state_t;

esp_err_t       reset_handler_init(void);
esp_err_t       reset_handler_start(void);
esp_err_t       reset_handler_confirm(void);
esp_err_t       reset_handler_abort(void);
void            reset_handler_tick(uint64_t now_ms);
bool            reset_handler_is_pending(void);
int             reset_handler_remaining_s(void);
reset_state_t   reset_handler_get_state(void);

#endif
