#include "driver_mcp3208.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "pin_map.h"

static const char *TAG = "mcp3208";

static spi_device_handle_t s_mcp3208_dev = NULL;

esp_err_t mcp3208_init(int cs_gpio)
{
    (void)cs_gpio;
    if (s_mcp3208_dev != NULL) return ESP_OK;

    gpio_config_t cs1_cfg = {
        .pin_bit_mask = (1ULL << PIN_ADC1_CS_GPIO) | (1ULL << PIN_ADC2_CS_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cs1_cfg);
    gpio_set_level(PIN_ADC1_CS_GPIO, 1);
    gpio_set_level(PIN_ADC2_CS_GPIO, 1);

    const spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1
    };

    esp_err_t err = spi_bus_add_device(SPI2_HOST, &devcfg, &s_mcp3208_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device falhou: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "MCP3208 init OK (CS manual GPIO41/42)");
    return ESP_OK;
}

esp_err_t mcp3208_read_channel(int cs_gpio, uint8_t channel, uint16_t *adc_value)
{
    if (channel > 7 || adc_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_mcp3208_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t tx_buf[3] = { 0x06, (uint8_t)(channel << 6), 0x00 };
    uint8_t rx_buf[3] = { 0 };

    spi_transaction_t t = {
        .length = 24,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf
    };

    gpio_set_level(cs_gpio, 0);
    esp_err_t err = spi_device_transmit(s_mcp3208_dev, &t);
    gpio_set_level(cs_gpio, 1);

    if (err != ESP_OK) return err;

    *adc_value = ((uint16_t)(rx_buf[1] & 0x0F) << 8) | rx_buf[2];
    return ESP_OK;
}
