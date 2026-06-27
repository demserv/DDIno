// @requirement RF-WEB-003 Operações de escrita validadas via command model
// @requirement RNF-SECURITY-001 Validação de comandos com dupla confirmação
#ifndef FIRMWARE_INCLUDE_COMMAND_MODEL_H
#define FIRMWARE_INCLUDE_COMMAND_MODEL_H

#include <stdbool.h>
#include <stdint.h>
#include "plug_model.h"

typedef enum {
    COMMAND_SOURCE_UI = 0,
    COMMAND_SOURCE_API
} command_source_t;

typedef enum {
    COMMAND_TYPE_NONE = 0,
    COMMAND_TYPE_SET_PLUG_STATE,
    COMMAND_TYPE_ACK_ALERT,
    COMMAND_TYPE_SET_MAINTENANCE_MODE
} command_type_t;

typedef struct {
    command_source_t source;
    command_type_t   type;
    uint64_t         timestamp;
    uint32_t         auth_context_id;
} command_header_t;

typedef struct {
    command_header_t header;
    plug_id_t        plug_id;
    bool             requested_on;
} command_set_plug_state_t;

typedef struct {
    command_header_t header;
    uint16_t         alert_id;
} command_ack_alert_t;

typedef struct {
    command_header_t header;
    bool             maintenance_mode;
} command_set_maintenance_mode_t;

#endif
