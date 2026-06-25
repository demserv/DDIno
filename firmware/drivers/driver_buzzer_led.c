// @requirement RF-LED-001 LED Verde (sistema OK)
// @requirement RF-LED-002 LED Amarelo (atenção/feed mode)
// @requirement RF-LED-003 LED Vermelho e padrão crítico
// @requirement RF-GLOBAL-003 Badge de estado sempre visível
#include "driver_buzzer_led.h"
#include "driver_mcp23017.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "buzzer_led";

#define MCP23017_ADDR      0x20
#define MCP23017_REG_IODIRB 0x01

#define LED_GREEN_BIT      (1 << 2)
#define LED_YELLOW_BIT     (1 << 3)
#define LED_RED_BIT        (1 << 4)
#define BUZZER_BIT         (1 << 5)

static uint8_t s_gpio_state = 0;

esp_err_t buzzer_led_init(void)
{
    uint8_t data[2] = {MCP23017_REG_IODIRB, 0x00};
    esp_err_t ret = i2c_master_write_to_device(I2C_NUM_0, MCP23017_ADDR, data, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;
    s_gpio_state = 0;
    ret = mcp23017_write_gpiob(0);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Buzzer and LEDs initialized");
    }
    return ret;
}

void led_set(led_color_t color)
{
    s_gpio_state &= ~(LED_GREEN_BIT | LED_YELLOW_BIT | LED_RED_BIT);
    switch (color) {
        case LED_GREEN:
            s_gpio_state |= LED_GREEN_BIT;
            break;
        case LED_YELLOW:
            s_gpio_state |= LED_YELLOW_BIT;
            break;
        case LED_RED:
            s_gpio_state |= LED_RED_BIT;
            break;
        default:
            break;
    }
    mcp23017_write_gpiob(s_gpio_state);
}

void buzzer_beep(uint32_t duration_ms)
{
    s_gpio_state |= BUZZER_BIT;
    mcp23017_write_gpiob(s_gpio_state);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    s_gpio_state &= ~BUZZER_BIT;
    mcp23017_write_gpiob(s_gpio_state);
}

void led_all_on(void)
{
    s_gpio_state |= (LED_GREEN_BIT | LED_YELLOW_BIT | LED_RED_BIT);
    mcp23017_write_gpiob(s_gpio_state);
}

void led_all_off(void)
{
    s_gpio_state &= ~(LED_GREEN_BIT | LED_YELLOW_BIT | LED_RED_BIT);
    mcp23017_write_gpiob(s_gpio_state);
}

void buzzer_led_alert(void)
{
    for (int i = 0; i < 3; i++) {
        s_gpio_state |= (BUZZER_BIT | LED_RED_BIT);
        mcp23017_write_gpiob(s_gpio_state);
        vTaskDelay(pdMS_TO_TICKS(100));
        s_gpio_state &= ~BUZZER_BIT;
        mcp23017_write_gpiob(s_gpio_state);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void buzzer_led_clear(void)
{
    s_gpio_state = 0;
    mcp23017_write_gpiob(s_gpio_state);
}
