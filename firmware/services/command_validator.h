#ifndef FIRMWARE_SERVICES_COMMAND_VALIDATOR_H
#define FIRMWARE_SERVICES_COMMAND_VALIDATOR_H

#include <stdbool.h>
#include <stdint.h>
#include "global_state.h"

typedef struct {
    bool allowed;
    bool requires_double_confirmation;
    const char *error_code;
} cmd_validation_t;

cmd_validation_t command_validator_can_toggle_plug(const global_state_t *gs, uint8_t plug_id, bool desired_on);
cmd_validation_t command_validator_can_set_config(const global_state_t *gs, const char *key);
cmd_validation_t command_validator_can_start_feed(const global_state_t *gs);
cmd_validation_t command_validator_can_restart(const global_state_t *gs);
cmd_validation_t command_validator_can_ack_alert(const global_state_t *gs, uint16_t alert_id);
cmd_validation_t command_validator_can_set_mode(const global_state_t *gs, uint8_t plug_id);
cmd_validation_t command_validator_can_calibrate(const global_state_t *gs);

#endif
