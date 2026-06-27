// @requirement RF-THERMAL-002 Filtro de média móvel para temperatura
#ifndef FIRMWARE_SERVICES_TEMP_FILTER_H
#define FIRMWARE_SERVICES_TEMP_FILTER_H

#include <stdint.h>
#include <stdbool.h>

#define TEMP_FILTER_MAX_WINDOW 20

void temp_filter_init(uint8_t window_size);
float temp_filter_update(float sample_c);
float temp_filter_get_current(void);
bool temp_filter_is_valid(void);
void temp_filter_reset(void);

#endif
