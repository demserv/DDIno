// @requirement RF-PLUG-001 Abstração de relés — rota única de comutação via driver_relay
// @requirement RF-PLUG-009 Religamento sequencial coordenado
// @requirement RF-PLUG-011 Dupla confirmação integrada
#include "relay_abstraction.h"

#include <string.h>

#include "esp_log.h"

#include "driver_relay.h"

static const char *TAG = "relay_abstr";

static relay_state_t s_states[RELAY_COUNT];

static const char *RELAY_NAMES[RELAY_COUNT] = {
    "P01-Aquecedor", "P02-Cooler", "P03", "P04", "P05",
    "P06", "P07", "P08", "P09", "P10"
};

esp_err_t relay_abstraction_init(void)
{
    memset(s_states, 0, sizeof(s_states));
    ESP_LOGI(TAG, "Relay abstraction initialized");
    return ESP_OK;
}

esp_err_t relay_abstraction_set(relay_id_t id, bool on)
{
    if (id >= RELAY_COUNT) return ESP_ERR_INVALID_ARG;
    if (s_states[id] == RELAY_STATE_BLOCKED) return ESP_ERR_INVALID_STATE;

    // @requirement RF-PLUG-010 Exclusao mutua P01 (Aquecedor) e P02 (Cooler)
    if (on) {
        if (id == RELAY_ID_P01 && s_states[RELAY_ID_P02] == RELAY_STATE_ON) {
            ESP_LOGW(TAG, "P01 negado: P02 (Cooler) esta ON");
            return ESP_ERR_INVALID_STATE;
        }
        if (id == RELAY_ID_P02 && s_states[RELAY_ID_P01] == RELAY_STATE_ON) {
            ESP_LOGW(TAG, "P02 negado: P01 (Aquecedor) esta ON");
            return ESP_ERR_INVALID_STATE;
        }
    }

    esp_err_t err = relay_set(id, on);
    if (err == ESP_OK) {
        s_states[id] = on ? RELAY_STATE_ON : RELAY_STATE_OFF;
    }
    return err;
}

esp_err_t relay_abstraction_set_blocked(relay_id_t id, bool blocked)
{
    if (id >= RELAY_COUNT) return ESP_ERR_INVALID_ARG;
    if (blocked) {
        relay_abstraction_set(id, false);
        s_states[id] = RELAY_STATE_BLOCKED;
    } else {
        s_states[id] = RELAY_STATE_OFF;
    }
    return ESP_OK;
}

relay_state_t relay_abstraction_get_state(relay_id_t id)
{
    if (id >= RELAY_COUNT) return RELAY_STATE_FAULT;
    return s_states[id];
}

bool relay_abstraction_is_on(relay_id_t id)
{
    return (id < RELAY_COUNT && s_states[id] == RELAY_STATE_ON);
}

bool relay_abstraction_is_blocked(relay_id_t id)
{
    return (id < RELAY_COUNT && s_states[id] == RELAY_STATE_BLOCKED);
}

esp_err_t relay_abstraction_all_off(void)
{
    esp_err_t err = relay_all_off();
    if (err == ESP_OK) {
        for (int i = 0; i < RELAY_COUNT; i++) {
            if (s_states[i] != RELAY_STATE_BLOCKED) {
                s_states[i] = RELAY_STATE_OFF;
            }
        }
    }
    return err;
}

esp_err_t relay_abstraction_all_on(void)
{
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (s_states[i] != RELAY_STATE_BLOCKED) {
            esp_err_t err = relay_abstraction_set((relay_id_t)i, true);
            if (err != ESP_OK) return err;
        }
    }
    return ESP_OK;
}

const char *relay_abstraction_get_name(relay_id_t id)
{
    if (id >= RELAY_COUNT) return "INVALID";
    return RELAY_NAMES[id];
}

