// @requirement RF-PLUG-001 Abstração de relés — rota única de comutação via driver_relay
// @requirement RF-PLUG-009 Religamento sequencial coordenado
// @requirement RF-PLUG-011 Dupla confirmação integrada
#include "relay_abstraction.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "driver_relay.h"
#include "global_state.h"
#include "hardware_config.h"

static const char *TAG = "relay_abstr";

static relay_state_t s_states[RELAY_COUNT];
static uint64_t s_last_off_ms[RELAY_COUNT];
static bool s_confirm_armed[RELAY_COUNT];

static const char *RELAY_NAMES[RELAY_COUNT] = {
    "P01-Aquecedor", "P02-Cooler", "P03", "P04", "P05",
    "P06", "P07", "P08", "P09", "P10"
};

/* @requirement RF-PLUG-011 P01 (Aquecedor) e P02 (Cooler) são relés críticos. */
static bool relay_is_critical(relay_id_t id)
{
    return (id == RELAY_ID_P01 || id == RELAY_ID_P02);
}

static bool relay_lockout_active(relay_id_t id)
{
#ifdef HW_RELAY_LOCKOUT_MS
    uint64_t now_ms = esp_timer_get_time() / 1000ULL;
    uint64_t last_off = s_last_off_ms[id];
    return last_off != 0 && ((now_ms - last_off) < HW_RELAY_LOCKOUT_MS);
#else
    (void)id;
    return false;
#endif
}

esp_err_t relay_abstraction_init(void)
{
    memset(s_states, 0, sizeof(s_states));
    memset(s_last_off_ms, 0, sizeof(s_last_off_ms));
    memset(s_confirm_armed, 0, sizeof(s_confirm_armed));
    ESP_LOGI(TAG, "Relay abstraction initialized");
    return ESP_OK;
}

void relay_abstraction_arm_critical_confirm(relay_id_t id)
{
    if (id < RELAY_COUNT) {
        s_confirm_armed[id] = true;
    }
}

esp_err_t relay_abstraction_set(relay_id_t id, bool on)
{
    if (id >= RELAY_COUNT) return ESP_ERR_INVALID_ARG;
    if (s_states[id] == RELAY_STATE_BLOCKED) return ESP_ERR_INVALID_STATE;

    /* @requirement RF-GLOBAL-002 / RF-PLUG-001 Rota única: bloqueio de segurança
     * antes de qualquer acionamento. SAFE_OFF/EMERGENCY força all_off e nega. */
    global_state_t gs;
    if (global_state_get_snapshot(&gs) != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    if (gs.system_state == SYSTEM_STATE_SAFE_OFF ||
        gs.system_state == SYSTEM_STATE_EMERGENCY) {
        (void)relay_abstraction_all_off();
        return ESP_ERR_INVALID_STATE;
    }

    if (on) {
        /* Self-test reprovado nunca permite ligar carga. */
        if (!gs.selftest_passed) {
            ESP_LOGW(TAG, "ON negado: self-test nao aprovado");
            return ESP_ERR_INVALID_STATE;
        }

        if (relay_lockout_active(id)) {
            ESP_LOGW(TAG, "ON negado: lockout ativo para rele %d", (int)id);
            return ESP_ERR_INVALID_STATE;
        }

        // @requirement RF-PLUG-010 Exclusao mutua P01 (Aquecedor) e P02 (Cooler)
        if (id == RELAY_ID_P01 && s_states[RELAY_ID_P02] == RELAY_STATE_ON) {
            ESP_LOGW(TAG, "P01 negado: P02 (Cooler) esta ON");
            return ESP_ERR_INVALID_STATE;
        }
        if (id == RELAY_ID_P02 && s_states[RELAY_ID_P01] == RELAY_STATE_ON) {
            ESP_LOGW(TAG, "P02 negado: P01 (Aquecedor) esta ON");
            return ESP_ERR_INVALID_STATE;
        }

        /* @requirement RF-PLUG-011 Relé crítico exige confirmação dupla prévia. */
        if (relay_is_critical(id) && !s_confirm_armed[id]) {
            ESP_LOGW(TAG, "ON negado: rele critico %d sem dupla confirmacao", (int)id);
            return ESP_ERR_INVALID_STATE;
        }
    }

    /* driver usa plug_id 1..10; abstração usa id 0..9. */
    esp_err_t err = relay_set((uint8_t)(id + 1), on);
    if (err == ESP_OK) {
        s_states[id] = on ? RELAY_STATE_ON : RELAY_STATE_OFF;
        if (!on) {
            s_last_off_ms[id] = esp_timer_get_time() / 1000ULL;
        }
    }
    /* Confirmação é de uso único, independente do resultado. */
    s_confirm_armed[id] = false;
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

