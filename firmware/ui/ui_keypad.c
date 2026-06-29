// @requirement RF-UI-INPUT-001 Entrada por keypad AD 5 botões
// @requirement RNF-HARDWARE-001 Keypad via MCP3208 CH3
#include "ui_keypad.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

#include "driver_mcp3208.h"
#include "pin_map.h"

static const char *TAG = "ui_keypad";

static uint32_t last_key_time = 0;
static uint16_t last_adc = 4096;
static int last_key = LV_KEY_ENTER;

static int map_adc_to_lvgl_key(uint16_t adc_value)
{
    if (adc_value < 100) return 0;
    if (adc_value < 600) return LV_KEY_UP;
    if (adc_value < 1200) return LV_KEY_DOWN;
    if (adc_value < 1800) return LV_KEY_LEFT;
    if (adc_value < 2400) return LV_KEY_RIGHT;
    if (adc_value < 3000) return LV_KEY_ENTER;
    if (adc_value < 3600) return LV_KEY_ESC;
    return 0x80;
}

bool ui_keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint32_t now = esp_timer_get_time() / 1000;
    if (now - last_key_time < 100) {
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return false;
    }

    uint16_t adc_value;
    esp_err_t ret = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc_value);
    if (ret != ESP_OK) {
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return false;
    }

    int key = map_adc_to_lvgl_key(adc_value);
    if (key == 0 || key == last_key) {
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return false;
    }

    last_key = key;
    last_key_time = now;

    data->state = LV_INDEV_STATE_PR;
    data->key = key;

    return false;
}

esp_err_t ui_keypad_init(void)
{
    ESP_LOGI(TAG, "Initializing AD keypad");

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = ui_keypad_read;
    lv_indev_drv_register(&indev_drv);

    ESP_LOGI(TAG, "AD keypad initialized");
    return ESP_OK;
}

