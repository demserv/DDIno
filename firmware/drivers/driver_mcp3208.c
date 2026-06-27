#include "driver_mcp3208.h"
#include "hal_spi.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "pin_map.h"

static const char *TAG = "mcp3208";

esp_err_t mcp3208_init(int cs_gpio)
{
    (void)cs_gpio;
    ESP_LOGI(TAG, "MCP3208 init delegado ao hal_spi (CS manual GPIO41/42)");
    return ESP_OK;
}

esp_err_t mcp3208_read_channel(int cs_gpio, uint8_t channel, uint16_t *adc_value)
{
    if (channel > 7 || adc_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t tx_buf[3] = { 0x06, (uint8_t)(channel << 6), 0x00 };
    uint8_t rx_buf[3] = { 0 };

    spi_transaction_t t = {
        .length = 24,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf
    };

    esp_err_t err = hal_spi_transaction_with_cs(HAL_SPI_DEVICE_MCP3208, cs_gpio, &t);
    if (err != ESP_OK) return err;

    *adc_value = ((uint16_t)(rx_buf[1] & 0x0F) << 8) | rx_buf[2];
    return ESP_OK;
}
