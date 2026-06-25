#ifndef FIRMWARE_DRIVERS_DS18B20_H
#define FIRMWARE_DRIVERS_DS18B20_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t ds18b20_init(void);
bool ds18b20_read(float *temp_c);
void ds18b20_reset_rom(void);

#endif
