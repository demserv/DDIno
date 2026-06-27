// @requirement RF-HEALTH-MATRIX-001..010, RF-WEB-005, RF-UI-DIAG-001
#include "health_matrix.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "health_mtx";
static health_entry_t s_entries[SUB_COUNT];

static const char *SUB_NAMES[SUB_COUNT] = {
    "TEMP", "PH", "LEVEL", "FLOW", "CURRENT", "VOLTAGE",
    "BUS_SPI", "BUS_I2C", "BUS_1WIRE",
    "NVS", "SD", "WIFI", "RTC", "RELAY_BOARD"
};

esp_err_t health_matrix_init(void)
{
    for (int i = 0; i < SUB_COUNT; i++) {
        s_entries[i].status = HEALTH_UNKNOWN;
        s_entries[i].last_ok_ms = 0;
        s_entries[i].fail_count = 0;
        s_entries[i].detail[0] = '\0';
    }
    ESP_LOGI(TAG, "Health matrix initialized (%d subsystems)", SUB_COUNT);
    return ESP_OK;
}

esp_err_t health_report(subsystem_id_t sub, health_status_t status, const char *reason)
{
    if (sub >= SUB_COUNT) return ESP_ERR_INVALID_ARG;
    s_entries[sub].status = status;
    if (status == HEALTH_OK) {
        s_entries[sub].last_ok_ms = 0;
    } else {
        s_entries[sub].fail_count++;
    }
    if (reason) {
        strncpy(s_entries[sub].detail, reason, sizeof(s_entries[sub].detail) - 1);
    }
    return ESP_OK;
}

health_status_t health_get(subsystem_id_t sub)
{
    if (sub >= SUB_COUNT) return HEALTH_UNKNOWN;
    return s_entries[sub].status;
}

health_status_t health_aggregate(void)
{
    bool any_failed = false;
    bool any_degraded = false;
    for (int i = 0; i < SUB_COUNT; i++) {
        if (s_entries[i].status == HEALTH_FAILED) return HEALTH_FAILED;
        if (s_entries[i].status == HEALTH_DEGRADED) any_degraded = true;
    }
    return any_degraded ? HEALTH_DEGRADED : HEALTH_OK;
}

const health_entry_t *health_get_entry(subsystem_id_t sub)
{
    if (sub >= SUB_COUNT) return NULL;
    return &s_entries[sub];
}

void health_matrix_update(void)
{
    health_aggregate();
}
