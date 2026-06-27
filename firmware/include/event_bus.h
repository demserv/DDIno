// @requirement RF-GLOBAL-002 Barramento de eventos para comunicação desacoplada entre módulos
// @requirement RF-UI-INPUT-001 Eventos de UI propagados via event_bus
#ifndef FIRMWARE_INCLUDE_EVENT_BUS_H
#define FIRMWARE_INCLUDE_EVENT_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t event_id_t;

#define EVENT_ID_NONE           0
#define EVENT_ID_SAFE_OFF       1
#define EVENT_ID_EMERGENCY      2
#define EVENT_ID_DEGRADED       3
#define EVENT_ID_NORMAL         4
#define EVENT_ID_ALERT_RAISED   5
#define EVENT_ID_ALERT_ACKED    6
#define EVENT_ID_PLUG_TOGGLED   7
#define EVENT_ID_FEED_START     8
#define EVENT_ID_FEED_END       9
#define EVENT_ID_CONFIG_CHANGED 10
#define EVENT_ID_WIZARD_DONE    11
#define EVENT_ID_RESET_START    12
#define EVENT_ID_TEMP_CRITICAL  13
#define EVENT_ID_OVER_CURRENT   14
#define EVENT_ID_ATO_OVERFLOW   15
#define EVENT_ID_SD_MOUNTED     16
#define EVENT_ID_SD_UNMOUNTED   17
#define EVENT_ID_WDT_TIMEOUT    18
#define EVENT_ID_BOOT_COMPLETE  19

typedef void (*event_callback_t)(event_id_t evt, void *data, void *user_ctx);

esp_err_t event_bus_init(void);
esp_err_t event_bus_subscribe(event_id_t evt, event_callback_t cb, void *user_ctx);
esp_err_t event_bus_unsubscribe(event_id_t evt, event_callback_t cb);
esp_err_t event_bus_publish(event_id_t evt, void *data);
esp_err_t event_bus_publish_isr(event_id_t evt, void *data);
void event_bus_process_pending(void);

#ifdef __cplusplus
}
#endif

#endif
