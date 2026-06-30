// @requirement RNF-HARDWARE-001 HAL de barramentos (I2C, SPI, UART)
#ifndef HAL_BUS_H
#define HAL_BUS_H

#include "esp_err.h"
#include "driver/i2c.h"

esp_err_t hal_bus_init_i2c(void);
esp_err_t hal_bus_init_spi(void);
esp_err_t hal_bus_init_uart_pzem(void);
esp_err_t hal_bus_init_all(void);

bool hal_i2c_lock(void);
void hal_i2c_unlock(void);

esp_err_t hal_i2c_master_write_to_device(i2c_port_t port, uint8_t addr,
                                         const uint8_t *data, size_t len,
                                         TickType_t timeout);
esp_err_t hal_i2c_master_write_read_device(i2c_port_t port, uint8_t addr,
                                           const uint8_t *write_buf, size_t write_size,
                                           uint8_t *read_buf, size_t read_size,
                                           TickType_t timeout);

#endif
