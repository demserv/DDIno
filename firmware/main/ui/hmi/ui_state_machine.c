// @requirement RF-GLOBAL-001 a RF-GLOBAL-002 Máquina de estados de UI
// @requirement RF-UI-OVERLAY-001 Overlays por estado global
#include <stddef.h>
#include "ui_state_machine.h"

void ui_state_machine_init(ui_state_machine_t *sm)
{
    sm->current_state = UI_SYSTEM_NORMAL;
    sm->on_enter = NULL;
    sm->on_exit = NULL;
}

void ui_state_machine_transition(ui_state_machine_t *sm, ui_system_state_t new_state)
{
    if (sm->current_state == new_state) return;
    if (sm->on_exit) sm->on_exit();
    sm->current_state = new_state;
    if (sm->on_enter) sm->on_enter();
}
