// @requirement RF-PLUG-001 Camada de abstração de relés — rota única de comutação
// @requirement RF-PLUG-011 Dupla confirmação para relés críticos via abstraction
#ifndef FIRMWARE_INCLUDE_RELAY_ABSTRACTION_H
#define FIRMWARE_INCLUDE_RELAY_ABSTRACTION_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t relay_id_t;
#define RELAY_ID_P01 0
#define RELAY_ID_P02 1
#define RELAY_ID_P03 2
#define RELAY_ID_P04 3
#define RELAY_ID_P05 4
#define RELAY_ID_P06 5
#define RELAY_ID_P07 6
#define RELAY_ID_P08 7
#define RELAY_ID_P09 8
#define RELAY_ID_P10 9
#define RELAY_COUNT 10

typedef enum {
    RELAY_STATE_OFF = 0,
    RELAY_STATE_ON = 1,
    RELAY_STATE_BLOCKED = 2,
    RELAY_STATE_FAULT = 3
} relay_state_t;

esp_err_t relay_abstraction_init(void);
esp_err_t relay_abstraction_set(relay_id_t id, bool on);
/* @requirement RF-PLUG-011 Arma confirmação dupla (uso único) para relé crítico. */
void relay_abstraction_arm_critical_confirm(relay_id_t id);
esp_err_t relay_abstraction_set_blocked(relay_id_t id, bool blocked);
relay_state_t relay_abstraction_get_state(relay_id_t id);
bool relay_abstraction_is_on(relay_id_t id);
bool relay_abstraction_is_blocked(relay_id_t id);
esp_err_t relay_abstraction_all_off(void);
esp_err_t relay_abstraction_all_on(void);
const char *relay_abstraction_get_name(relay_id_t id);

#ifdef __cplusplus
}
#endif

#endif
