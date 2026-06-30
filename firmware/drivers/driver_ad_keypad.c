// @requirement RF-GLOBAL-003 Entrada por keypad como fallback local
// @requirement RNF-HARDWARE-001 AD Keypad 5V com condicionamento via MCP3208 CH3
#include "driver_ad_keypad.h"

#include "esp_log.h"

#include "driver_mcp3208.h"
#include "pin_map.h"

static const char *TAG = "ad_keypad";

#define KEYPAD_ADC_RANGE (4095)

static ad_keypad_callback_t s_callback = NULL;

ad_keypad_key_t ad_keypad_read(void)
{
    uint16_t adc = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc);
    if (err != ESP_OK) return KEY_NONE;

    if (adc > HW_AD_KEYPAD_THRESH_NONE) return KEY_NONE;

    if (adc > HW_AD_KEYPAD_THRESH_FEED) return KEY_FEED;
    if (adc > HW_AD_KEYPAD_THRESH_MENU) return KEY_MENU;
    if (adc > HW_AD_KEYPAD_THRESH_ESC)  return KEY_ESC;
    if (adc > HW_AD_KEYPAD_THRESH_ENTER)  return KEY_ENTER;
    if (adc > HW_AD_KEYPAD_THRESH_RIGHT)  return KEY_RIGHT;
    if (adc > HW_AD_KEYPAD_THRESH_LEFT)   return KEY_LEFT;
    if (adc > HW_AD_KEYPAD_THRESH_DOWN)   return KEY_DOWN;
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

