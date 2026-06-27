#include "global_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static global_state_t s_gs;
static SemaphoreHandle_t s_mutex = NULL;

static void mutex_lock(void)
{
    if (s_mutex) xSemaphoreTake(s_mutex, portMAX_DELAY);
}

static void mutex_unlock(void)
{
    if (s_mutex) xSemaphoreGive(s_mutex);
}

void global_state_init(void)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
    }
    mutex_lock();
    memset(&s_gs, 0, sizeof(s_gs));
    s_gs.system_state = SYSTEM_STATE_NORMAL;
    s_gs.safeoff_reason = SAFEOFF_REASON_NONE;
    s_gs.health_check_interval_s = 60;
    mutex_unlock();
}

const global_state_t* global_state_get_snapshot(void)
{
    return &s_gs;
}

esp_err_t global_state_transition(system_state_t next_state, safeoff_reason_t reason,
                                   const char *source_alm, const char *source_module,
                                   uint64_t now_s)
{
    mutex_lock();
    system_state_t prev = s_gs.system_state;

    if (prev == next_state) {
        mutex_unlock();
        return ESP_OK;
    }

    if (prev > next_state && prev != SYSTEM_STATE_EMERGENCY) {
        if (next_state < SYSTEM_STATE_SAFE_OFF) {
            mutex_unlock();
            return ESP_ERR_INVALID_ARG;
        }
    }

    s_gs.system_state = next_state;
    if (next_state == SYSTEM_STATE_SAFE_OFF) {
        s_gs.safeoff_reason = reason;
        if (source_alm) {
            strncpy(s_gs.safeoff_source_alm, source_alm, sizeof(s_gs.safeoff_source_alm) - 1);
        }
        if (source_module) {
            strncpy(s_gs.safeoff_entered_at, source_module, sizeof(s_gs.safeoff_entered_at) - 1);
        }
        s_gs.electric_ok = false;
    }
    if (next_state == SYSTEM_STATE_EMERGENCY) {
        s_gs.electric_ok = false;
    }
    if (next_state == SYSTEM_STATE_NORMAL) {
        s_gs.safeoff_reason = SAFEOFF_REASON_NONE;
        s_gs.safeoff_source_alm[0] = '\0';
        s_gs.electric_ok = true;
    }

    mutex_unlock();
    return ESP_OK;
}

global_state_t* global_state_get_write_ptr(void)
{
    return &s_gs;
}