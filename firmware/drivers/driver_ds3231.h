// @requirement RF-TIME-001 DS3231 RTC via I2C como fonte de tempo principal
#ifndef FIRMWARE_DRIVERS_DRIVER_DS3231_H
#define FIRMWARE_DRIVERS_DRIVER_DS3231_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} ds3231_time_t;

esp_err_t ds3231_init(void);
esp_err_t ds3231_set_time(const ds3231_time_t *t);
esp_err_t ds3231_get_time(ds3231_time_t *t);
bool ds3231_is_running(void);
float ds3231_get_temp(void);

#endif
