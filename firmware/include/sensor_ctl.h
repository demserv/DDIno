// @requirement RF-SENSOR-001 a RF-SENSOR-003 Medição coordenada de temperatura, corrente e potência
#ifndef FIRMWARE_INCLUDE_SENSOR_CTL_H
#define FIRMWARE_INCLUDE_SENSOR_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SENSOR_TYPE_TEMPERATURE = 0,
    SENSOR_TYPE_CURRENT,
    SENSOR_TYPE_POWER,
    SENSOR_TYPE_COUNT
} sensor_ctl_type_t;

typedef struct {
    float temperature_c;
    float current_a;
    float power_w;
} sensor_ctl_snapshot_t;

const char *sensor_ctl_get_name(sensor_ctl_type_t type);
esp_err_t sensor_ctl_init(void);
esp_err_t sensor_ctl_read(sensor_ctl_type_t type, float *out);
esp_err_t sensor_ctl_read_all(sensor_ctl_snapshot_t *snap);
int sensor_ctl_get_last_error_count(sensor_ctl_type_t type);
void sensor_ctl_reset_error_count(sensor_ctl_type_t type);

#ifdef __cplusplus
}
#endif

#endif
