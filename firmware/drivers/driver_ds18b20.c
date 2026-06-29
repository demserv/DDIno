// @requirement RF-THERMAL-001 Leitura contínua com validação robusta
// @requirement RF-THERMAL-009 CRC obrigatório, rejeição 85°C, timeout ALM-013, moving average 3
#include "driver_ds18b20.h"
#include "hardware_config.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "pin_map.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "alert_manager.h"

static const char *TAG = "ds18b20";

static uint8_t crc8_dallas(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            byte >>= 1;
        }
    }
    return crc;
}

#define DS18B20_GPIO           PIN_DS18B20_DQ_GPIO
#define DS18B20_TIMEOUT_MS     1000

static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;

static bool ow_reset(void)
{
    bool presence = false;
    portENTER_CRITICAL(&s_spinlock);
    gpio_set_level(DS18B20_GPIO, 0);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    esp_rom_delay_us(480);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(70);
    if (gpio_get_level(DS18B20_GPIO) == 0) {
        presence = true;
    }
    esp_rom_delay_us(410);
    portEXIT_CRITICAL(&s_spinlock);
    return presence;
}

static void ow_write_bit(bool bit)
{
    portENTER_CRITICAL(&s_spinlock);
    gpio_set_level(DS18B20_GPIO, 0);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    if (bit) {
        esp_rom_delay_us(6);
        gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
        esp_rom_delay_us(64);
    } else {
        esp_rom_delay_us(60);
        gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
        esp_rom_delay_us(10);
    }
    portEXIT_CRITICAL(&s_spinlock);
}

static bool ow_read_bit(void)
{
    bool bit;
    portENTER_CRITICAL(&s_spinlock);
    gpio_set_level(DS18B20_GPIO, 0);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    esp_rom_delay_us(6);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(9);
    bit = (gpio_get_level(DS18B20_GPIO) != 0);
    esp_rom_delay_us(55);
    portEXIT_CRITICAL(&s_spinlock);
    return bit;
}

static void ow_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t ow_read_byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (ow_read_bit()) {
            byte |= (1 << i);
        }
    }
    return byte;
}

esp_err_t ds18b20_init(void)
{
    gpio_reset_pin(DS18B20_GPIO);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    ESP_LOGI(TAG, "DS18B20 initialized on GPIO %d", DS18B20_GPIO);
    return ESP_OK;
}

bool ds18b20_read(float *temp_c)
{
    uint8_t scratchpad[9];
    uint64_t start = esp_timer_get_time() / USEC_PER_MSEC;

    if (!ow_reset()) {
        ESP_LOGW(TAG, "DS18B20 sem resposta (ow_reset falhou)");
        alert_manager_raise(ALM_013, true, start / USEC_PER_MSEC);
        return false;
    }
    ow_write_byte(0xCC);
    ow_write_byte(0x44);
    vTaskDelay(pdMS_TO_TICKS(750));
    if ((esp_timer_get_time() / USEC_PER_MSEC - start) > DS18B20_TIMEOUT_MS) {
        ESP_LOGW(TAG, "DS18B20 timeout na conversao");
        alert_manager_raise(ALM_013, true, esp_timer_get_time() / USEC_PER_SEC);
        return false;
    }
    if (!ow_reset()) {
        ESP_LOGW(TAG, "DS18B20 sem resposta apos conversao");
        alert_manager_raise(ALM_013, true, esp_timer_get_time() / USEC_PER_SEC);
        return false;
    }
    ow_write_byte(0xCC);
    ow_write_byte(0xBE);
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = ow_read_byte();
    }
    if (crc8_dallas(scratchpad, 8) != scratchpad[8]) {
        ESP_LOGW(TAG, "CRC mismatch on scratchpad");
        alert_manager_raise(ALM_013, true, esp_timer_get_time() / USEC_PER_SEC);
        return false;
    }
    int16_t raw = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);
    *temp_c = raw * 0.0625f;
    if (*temp_c > 84.9f && *temp_c < 85.1f) {
        ESP_LOGW(TAG, "Rejeitando leitura 85C (power-on reset)");
        alert_manager_raise(ALM_013, true, esp_timer_get_time() / USEC_PER_SEC);
        return false;
    }
    return true;
}

void ds18b20_reset_rom(void)
{
    if (ow_reset()) {
        ow_write_byte(0xCC);
    }
}
