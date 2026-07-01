#ifndef DRIVERS_DRIVER_PH_SENSOR_H
#define DRIVERS_DRIVER_PH_SENSOR_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t ph_sensor_init(void);
esp_err_t ph_sensor_read(float *ph_out, bool *valid_out);
bool ph_sensor_get_last(float *ph_out);
bool ph_sensor_is_ok(void);

#endif
