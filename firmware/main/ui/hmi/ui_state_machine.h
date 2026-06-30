// @requirement RF-GLOBAL-001 FSM de estados da UI
#ifndef HMI_UI_STATE_MACHINE_H
#define HMI_UI_STATE_MACHINE_H

#include "ui_types.h"

typedef void (*ui_state_enter_fn_t)(void);
typedef void (*ui_state_exit_fn_t)(void);

typedef struct {
    ui_system_state_t current_state;
    ui_state_enter_fn_t on_enter;
    ui_state_exit_fn_t on_exit;
} ui_state_machine_t;

void ui_state_machine_init(ui_state_machine_t *sm);
void ui_state_machine_transition(ui_state_machine_t *sm, ui_system_state_t new_state);

#endif
