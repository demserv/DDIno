#ifndef FIRMWARE_HAL_SPI_H
#define FIRMWARE_HAL_SPI_H

#include "esp_err.h"
#include "driver/spi_master.h"
#include <stdint.h>

typedef enum {
    HAL_SPI_DEVICE_TFT = 0,
    HAL_SPI_DEVICE_TOUCH,
    HAL_SPI_DEVICE_SD,
    HAL_SPI_DEVICE_MCP3208,
    HAL_SPI_DEVICE_COUNT
} hal_spi_device_t;

esp_err_t hal_spi_init(void);
esp_err_t hal_spi_transaction(hal_spi_device_t device, spi_transaction_t *trans);
esp_err_t hal_spi_transaction_polling(hal_spi_device_t device, spi_transaction_t *trans);
esp_err_t hal_spi_transaction_with_cs(hal_spi_device_t device, int cs_gpio, spi_transaction_t *trans);
esp_err_t hal_spi_transaction_with_cs_polling(hal_spi_device_t device, int cs_gpio, spi_transaction_t *trans);
int hal_spi_get_cs_gpio(hal_spi_device_t device);
spi_device_handle_t hal_spi_get_handle(hal_spi_device_t device);

#endif
