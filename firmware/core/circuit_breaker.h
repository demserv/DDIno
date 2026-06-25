#ifndef FIRMWARE_CORE_CIRCUIT_BREAKER_H
#define FIRMWARE_CORE_CIRCUIT_BREAKER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    CB_STATE_CLOSED,
    CB_STATE_OPEN,
    CB_STATE_HALF_OPEN
} cb_state_t;

typedef enum {
    CB_BUS_I2C,
    CB_BUS_SPI_ADC,
    CB_BUS_SPI_DISPLAY,
    CB_BUS_SPI_SD,
    CB_BUS_UART_PZEM,
    CB_BUS_DS18B20,
    CB_COUNT
} cb_bus_id_t;

typedef struct {
    cb_bus_id_t id;
    cb_state_t state;
    uint32_t failure_count;
    uint32_t success_count;
    uint64_t last_failure_ms;
    uint64_t opened_at_ms;
    uint32_t failure_threshold;
    uint32_t success_threshold;
    uint32_t half_open_timeout_ms;
    uint32_t recover_timeout_ms;
} circuit_breaker_t;

void circuit_breaker_init(void);
void circuit_breaker_record_success(cb_bus_id_t id);
void circuit_breaker_record_failure(cb_bus_id_t id);
bool circuit_breaker_is_available(cb_bus_id_t id);
cb_state_t circuit_breaker_get_state(cb_bus_id_t id);
void circuit_breaker_reset(cb_bus_id_t id);
void circuit_breaker_update(void);
uint32_t circuit_breaker_open_count(cb_bus_id_t id);

#endif
