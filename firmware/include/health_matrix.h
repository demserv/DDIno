// @requirement RF-HEALTH-MATRIX-001..010, RF-WEB-005, RF-UI-DIAG-001
#ifndef FIRMWARE_INCLUDE_HEALTH_MATRIX_H
#define FIRMWARE_INCLUDE_HEALTH_MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HEALTH_OK = 0,
    HEALTH_DEGRADED,
    HEALTH_FAILED,
    HEALTH_OPEN,
    HEALTH_HALF_OPEN,
    HEALTH_CLOSED,
    HEALTH_UNKNOWN
} health_status_t;

typedef enum {
    SUB_SENSOR_TEMP = 0,
    SUB_SENSOR_PH,
    SUB_SENSOR_LEVEL,
    SUB_SENSOR_FLOW,
    SUB_SENSOR_CURRENT,
    SUB_SENSOR_VOLTAGE,
    SUB_BUS_SPI,
    SUB_BUS_I2C,
    SUB_BUS_1WIRE,
    SUB_NVS,
    SUB_SD,
    SUB_WIFI,
    SUB_RTC,
    SUB_RELAY_BOARD,
    SUB_UI,
    SUB_WEB_SECURITY,
    SUB_WDT,
    SUB_COUNT
} subsystem_id_t;

typedef struct {
    health_status_t status;
    uint32_t last_ok_ms;
    uint32_t fail_count;
    char detail[64];
} health_entry_t;

esp_err_t health_matrix_init(void);
esp_err_t health_report(subsystem_id_t sub, health_status_t status, const char *reason);
health_status_t health_get(subsystem_id_t sub);
health_status_t health_aggregate(void);
const health_entry_t *health_get_entry(subsystem_id_t sub);
void health_matrix_update(void);

#ifdef __cplusplus
}
#endif

#endif
