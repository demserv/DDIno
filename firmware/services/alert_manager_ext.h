#ifndef FIRMWARE_SERVICES_ALERT_MANAGER_EXT_H
#define FIRMWARE_SERVICES_ALERT_MANAGER_EXT_H

#include <stdint.h>
#include <stdbool.h>
#include "alert_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACK_STAGE_NONE     0
#define ACK_STAGE_FIRST    1
#define ACK_STAGE_CONFIRMED 2

typedef struct {
    int16_t   alm_id;
    uint8_t   ack_stage;
    uint64_t  first_ack_ts;
    bool      requires_double_ack;
} alert_ack_state_t;

bool alert_manager_ext_ack_critical(int16_t alm_id, uint64_t ts);
uint8_t alert_manager_ext_get_ack_stage(int16_t alm_id);
bool alert_manager_ext_is_critical_and_pending(int16_t alm_id);
void alert_manager_ext_reset_ack_stage(int16_t alm_id);
bool alert_manager_ext_double_ack_required(int16_t alm_id);

#ifdef __cplusplus
}
#endif

#endif
