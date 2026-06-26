#include "ui_events.h"

void ui_events_emit(ui_event_t event)
{
    /* TODO: Encaminhar evento para command_validator quando disponivel. */
    /* A UI nao deve executar logica de negocio diretamente. */
    (void)event;
}
