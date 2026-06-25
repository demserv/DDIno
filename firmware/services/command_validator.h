#ifndef FIRMWARE_SERVICES_COMMAND_VALIDATOR_H
#define FIRMWARE_SERVICES_COMMAND_VALIDATOR_H

#include <stdbool.h>
#include <stdint.h>
#include "global_state.h"

typedef enum {
    CMD_TARGET_PLUG = 0,
    CMD_TARGET_CONFIG,
    CMD_TARGET_MAINTENANCE
} cmd_target_t;

typedef struct {
    bool allowed;
    bool requires_double_confirmation;
    const char *error_code;
} cmd_validation_t;

cmd_validation_t command_validator_can_toggle_plug(const global_state_t *gs, uint8_t plug_id, bool desired_on);

#endif
