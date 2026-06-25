#include "hal_bus.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "pin_map.h"

static const char *TAG = "hal_bus";

esp_err_t hal_bus_init_i2c(void)
{
    const i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA_GPIO,
        .scl_io_num = PIN_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    esp_err_t err = i2c_param_config(I2C_NUM_0, &cfg);
    if (err != ESP_OK) return err;
    err = i2c_driver_install(I2C_NUM_0, cfg.mode, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) return ESP_OK;
    return err;
}

esp_err_t hal_bus_init_spi(void)
{
    const spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_SPI_MOSI_GPIO,
        .miso_io_num = PIN_SPI_MISO_GPIO,
        .sclk_io_num = PIN_SPI_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err == ESP_ERR_INVALID_STATE) return ESP_OK;
    return err;
}

esp_err_t hal_bus_init_uart_pzem(void)
{
    const uart_config_t uart_cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB
    };
    esp_err_t err = uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = uart_param_config(UART_NUM_1, &uart_cfg);
    if (err != ESP_OK) return err;
    return uart_set_pin(UART_NUM_1, PIN_PZEM_TX_GPIO, PIN_PZEM_RX_GPIO,
                        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

esp_err_t hal_bus_init_all(void)
{
    esp_err_t err = hal_bus_init_i2c();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init falhou: %s", esp_err_to_name(err));
        return err;
    }
    err = hal_bus_init_spi();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI init falhou: %s", esp_err_to_name(err));
        return err;
    }
    err = hal_bus_init_uart_pzem();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART PZEM init falhou: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Barramentos base inicializados (I2C/SPI/UART)");
    return ESP_OK;
}
