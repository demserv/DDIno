// @requirement RF-UI-INPUT-001 Eventos de HMI desacoplados
#ifndef HMI_UI_EVENTS_H
#define HMI_UI_EVENTS_H

#include <stdint.h>

typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_REQUEST_FEED_MODE,
    UI_EVENT_REQUEST_EXIT_FEED_MODE,
    UI_EVENT_REQUEST_ACK_ALERT,
    UI_EVENT_REQUEST_MUTE,
    UI_EVENT_REQUEST_SAVE_THERMAL_CONFIG,
    UI_EVENT_REQUEST_PLUG_ACTION,
    UI_EVENT_REQUEST_SAFE_REBOOT,
    UI_EVENT_REQUEST_FACTORY_RESET,
    UI_EVENT_NAVIGATE_HOME,
    UI_EVENT_NAVIGATE_BACK
} ui_event_t;

/* @requirement RF-UI-MUTE-001 Opções normativas de duração do MUTE. */
typedef enum {
    UI_MUTE_DURATION_5MIN = 0,
    UI_MUTE_DURATION_10MIN,
    UI_MUTE_DURATION_15MIN,
    UI_MUTE_DURATION_UNTIL_ACK
} ui_mute_duration_t;

void ui_events_emit(ui_event_t event);
void ui_events_ack_alert(int16_t alm_id);
/* Seleciona a duração aplicada no próximo UI_EVENT_REQUEST_MUTE. */
void ui_events_set_mute_duration(ui_mute_duration_t duration);
ui_mute_duration_t ui_events_get_mute_duration(void);

#endif
