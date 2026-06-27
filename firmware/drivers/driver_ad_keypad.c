// @requirement RF-GLOBAL-003 Entrada por keypad como fallback local
// @requirement RNF-HARDWARE-001 AD Keypad 5V com condicionamento via MCP3208 CH3
#include "driver_ad_keypad.h"
#include "driver_mcp3208.h"
#include "pin_map.h"
#include "esp_log.h"

static const char *TAG = "ad_keypad";

#define KEYPAD_ADC_RANGE (4095)

static ad_keypad_callback_t s_callback = NULL;

ad_keypad_key_t ad_keypad_read(void)
{
    uint16_t adc = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc);
    if (err != ESP_OK) return KEY_NONE;

    if (adc > 4000) return KEY_NONE;

    if (adc > 3600) return KEY_FEED;
    if (adc > 3000) return KEY_MENU;
    if (adc > 2400) return KEY_ESC;
    if (adc > 1800) return KEY_ENTER;
    if (adc > 1200) return KEY_RIGHT;
    if (adc > 600)  return KEY_LEFT;
    if (adc > 100)  return KEY_DOWN;
    return KEY_UP;
}

esp_err_t ad_keypad_init(ad_keypad_callback_t callback)
{
    s_callback = callback;
    ESP_LOGI(TAG, "AD Keypad init (MCP3208 CH3)");
    return ESP_OK;
}

void ad_keypad_set_callback(ad_keypad_callback_t callback)
{
    s_callback = callback;
}
