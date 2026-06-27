// @requirement RF-PEND-001, RF-PEND-002, RB-PEND-001..003 Ações pendentes persistentes
#ifndef FIRMWARE_INCLUDE_PENDING_ACTIONS_H
#define FIRMWARE_INCLUDE_PENDING_ACTIONS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PENDING_SEND_TELEMETRY = 0,
    PENDING_EXPORT_CONFIG,
    PENDING_OTA_CHECK
} pending_action_type_t;

esp_err_t pending_actions_init(void);
esp_err_t pending_actions_enqueue(pending_action_type_t type, const uint8_t *data, uint32_t len);
esp_err_t pending_actions_dequeue(pending_action_type_t *type, uint8_t *data, uint32_t *len);
int pending_actions_count(void);
esp_err_t pending_actions_process_all(void);

#ifdef __cplusplus
}
#endif

#endif
