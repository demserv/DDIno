// @requirement RNF-HARDWARE-001 HAL de barramentos (I2C, SPI, UART)
#ifndef FIRMWARE_HAL_HAL_BUS_H
#define FIRMWARE_HAL_HAL_BUS_H

#include "esp_err.h"

esp_err_t hal_bus_init_i2c(void);
esp_err_t hal_bus_init_spi(void);
esp_err_t hal_bus_init_uart_pzem(void);
esp_err_t hal_bus_init_all(void);

#endif
