// @requirement RF-UI-INPUT-001 Sistema de eventos desacoplados para ações de usuário
// @requirement RF-UI-MUTE-001 Evento MUTE
#include "ui_events.h"

#include "command_validator.h"
#include "global_state.h"

extern global_state_t g_gs;

void ui_events_emit(ui_event_t event)
{
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) return;
    if (g_gs.monitor_only_mode && event == UI_EVENT_REQUEST_PLUG_ACTION) return;
    (void)event;
}
