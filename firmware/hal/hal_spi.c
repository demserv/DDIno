#include "hal_spi.h"
#include "pin_map.h"
#include "hardware_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "hal_spi";

static SemaphoreHandle_t s_spi_mutex = NULL;
static spi_device_handle_t s_devices[HAL_SPI_DEVICE_COUNT];

static const struct {
    int cs_gpio;
    int clock_hz;
    int mode;
    int flags;
} s_device_config[HAL_SPI_DEVICE_COUNT] = {
    [HAL_SPI_DEVICE_TFT]     = { .cs_gpio = PIN_TFT_CS_GPIO,   .clock_hz = HW_SPI_CLK_TFT_HZ,    .mode = 0, .flags = SPI_DEVICE_HALFDUPLEX },
    [HAL_SPI_DEVICE_TOUCH]   = { .cs_gpio = PIN_TOUCH_CS_GPIO, .clock_hz = HW_SPI_CLK_TOUCH_HZ,  .mode = 0, .flags = SPI_DEVICE_HALFDUPLEX },
    [HAL_SPI_DEVICE_SD]      = { .cs_gpio = PIN_SD_CS_GPIO,    .clock_hz = HW_SPI_CLK_SD_HZ,     .mode = 0, .flags = 0 },
    [HAL_SPI_DEVICE_MCP3208] = { .cs_gpio = -1,                .clock_hz = HW_SPI_CLK_MCP3208_HZ, .mode = 0, .flags = 0 },
};

static const int s_mcp3208_cs_pins[] = { PIN_ADC1_CS_GPIO, PIN_ADC2_CS_GPIO };
static const int s_mcp3208_cs_count = 2;

static bool s_mcp3208_cs_inited = false;

esp_err_t hal_spi_init(void)
{
    if (s_spi_mutex != NULL) return ESP_OK;

    s_spi_mutex = xSemaphoreCreateMutex();
    if (s_spi_mutex == NULL) {
        ESP_LOGE(TAG, "Falha ao criar mutex SPI");
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < HAL_SPI_DEVICE_COUNT; i++) {
        if (s_device_config[i].cs_gpio < 0) continue;

        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = s_device_config[i].clock_hz,
            .mode = s_device_config[i].mode,
            .spics_io_num = s_device_config[i].cs_gpio,
            .queue_size = HW_SPI_QUEUE_DEPTH,
            .flags = s_device_config[i].flags,
        };

        esp_err_t err = spi_bus_add_device(SPI2_HOST, &devcfg, &s_devices[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "spi_bus_add_device falhou device %d: %s", i, esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "SPI device %d init (CS=%d, clk=%d Hz, flags=0x%x)",
                 i, s_device_config[i].cs_gpio,
                 s_device_config[i].clock_hz, s_device_config[i].flags);
    }

    {
        gpio_config_t cs_cfg = {
            .pin_bit_mask = 0,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        for (int j = 0; j < s_mcp3208_cs_count; j++) {
            cs_cfg.pin_bit_mask |= (1ULL << s_mcp3208_cs_pins[j]);
        }
        gpio_config(&cs_cfg);
        for (int j = 0; j < s_mcp3208_cs_count; j++) {
            gpio_set_level(s_mcp3208_cs_pins[j], 1);
        }
        s_mcp3208_cs_inited = true;
        ESP_LOGI(TAG, "MCP3208 CS pins configurados (manual CS)");
    }

    ESP_LOGI(TAG, "SPI barramento com mutex inicializado");
    return ESP_OK;
}

static esp_err_t do_transaction(hal_spi_device_t device, spi_transaction_t *trans, bool polling)
{
    if (s_spi_mutex == NULL) return ESP_ERR_INVALID_STATE;
    if (device >= HAL_SPI_DEVICE_COUNT) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(s_spi_mutex, pdMS_TO_TICKS(HW_SPI_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err;
    if (polling) {
        err = spi_device_polling_transmit(s_devices[device], trans);
    } else {
        err = spi_device_transmit(s_devices[device], trans);
    }

    xSemaphoreGive(s_spi_mutex);
    return err;
}

esp_err_t hal_spi_transaction(hal_spi_device_t device, spi_transaction_t *trans)
{
    return do_transaction(device, trans, false);
}

esp_err_t hal_spi_transaction_polling(hal_spi_device_t device, spi_transaction_t *trans)
{
    return do_transaction(device, trans, true);
}

esp_err_t hal_spi_transaction_with_cs(hal_spi_device_t device, int cs_gpio, spi_transaction_t *trans)
{
    if (s_spi_mutex == NULL) return ESP_ERR_INVALID_STATE;
    if (device >= HAL_SPI_DEVICE_COUNT) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(s_spi_mutex, pdMS_TO_TICKS(HW_SPI_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    gpio_set_level(cs_gpio, 0);
    esp_err_t err = spi_device_transmit(s_devices[device], trans);
    gpio_set_level(cs_gpio, 1);

    xSemaphoreGive(s_spi_mutex);
    return err;
}

esp_err_t hal_spi_transaction_with_cs_polling(hal_spi_device_t device, int cs_gpio, spi_transaction_t *trans)
{
    if (s_spi_mutex == NULL) return ESP_ERR_INVALID_STATE;
    if (device >= HAL_SPI_DEVICE_COUNT) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(s_spi_mutex, pdMS_TO_TICKS(HW_SPI_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    gpio_set_level(cs_gpio, 0);
    esp_err_t err = spi_device_polling_transmit(s_devices[device], trans);
    gpio_set_level(cs_gpio, 1);

    xSemaphoreGive(s_spi_mutex);
    return err;
}

int hal_spi_get_cs_gpio(hal_spi_device_t device)
{
    if (device >= HAL_SPI_DEVICE_COUNT) return -1;
    return s_device_config[device].cs_gpio;
}

spi_device_handle_t hal_spi_get_handle(hal_spi_device_t device)
{
    if (device >= HAL_SPI_DEVICE_COUNT) return NULL;
    return s_devices[device];
}
