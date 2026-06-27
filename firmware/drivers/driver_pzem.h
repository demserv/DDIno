// @requirement RF-ENERGY-001 PZEM-004T v4.0 Modbus RTU via UART
#ifndef DRIVER_PZEM_H
#define DRIVER_PZEM_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    float voltage_v;
    float current_a;
    float power_w;
    float energy_wh;
    float frequency_hz;
    float pf;
    bool  valid;
    uint64_t last_read_ms;
} pzem_data_t;

esp_err_t pzem_init(void);
esp_err_t pzem_read_all(pzem_data_t *data);
esp_err_t pzem_reset_energy(void);

#endif
