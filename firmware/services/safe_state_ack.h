#ifndef FIRMWARE_SERVICES_SAFE_STATE_ACK_H
#define FIRMWARE_SERVICES_SAFE_STATE_ACK_H

#include <stdbool.h>
#include <stdint.h>
#include "global_state.h"

void safe_state_ack_on_enter_safeoff(const global_state_t *gs);
void safe_state_ack_on_enter_emergency(void);
void safe_state_ack_on_alert_ack(int16_t alm_id, uint64_t now_s);
bool safe_state_ack_manual_received(const global_state_t *gs);

#endif
