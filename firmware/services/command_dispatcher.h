#ifndef SERVICES_COMMAND_DISPATCHER_H
#define SERVICES_COMMAND_DISPATCHER_H

#include "esp_err.h"
#include "global_state.h"
#include "command_model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef esp_err_t (*command_handler_t)(void *ctx);

typedef struct {
    command_type_t    type;
    command_handler_t handler;
    void             *ctx;
} command_entry_t;

esp_err_t command_dispatch_execute(const command_entry_t *entry, const global_state_t *gs);

esp_err_t command_dispatch_toggle_plug(const global_state_t *gs, uint8_t plug_id, bool desired_on);
esp_err_t command_dispatch_ack_alert(const global_state_t *gs, uint16_t alert_id);
esp_err_t command_dispatch_start_feed(const global_state_t *gs);
esp_err_t command_dispatch_set_mode(const global_state_t *gs, uint8_t plug_id);

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /* SERVICES_COMMAND_DISPATCHER_H */