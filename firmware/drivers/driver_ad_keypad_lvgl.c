/* @requirement RF-UI-INPUT-001 a RF-UI-INPUT-003 keypad analógico */
// @requirement RF-UI-INPUT-001 Entrada por keypad AD 5 botões
// @requirement RNF-HARDWARE-001 Keypad via MCP3208 CH3
#include "driver_ad_keypad_lvgl.h"
#include "hardware_config.h"
#include "pin_map.h"
#include "driver_mcp3208.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "driver_ad_keypad_lvgl";

static uint32_t last_key_time = 0;
static uint16_t last_adc = 4096;
static int last_key = LV_KEY_ENTER;

static int map_adc_to_lvgl_key(uint16_t adc_value)
{
    if (adc_value < HW_AD_KEYPAD_THRESH_DOWN) return 0;
    if (adc_value < HW_AD_KEYPAD_THRESH_LEFT) return LV_KEY_UP;
    if (adc_value < HW_AD_KEYPAD_THRESH_RIGHT) return LV_KEY_DOWN;
    if (adc_value < HW_AD_KEYPAD_THRESH_ENTER) return LV_KEY_LEFT;
    if (adc_value < HW_AD_KEYPAD_THRESH_ESC) return LV_KEY_RIGHT;
    if (adc_value < HW_AD_KEYPAD_THRESH_MENU) return LV_KEY_ENTER;
    if (adc_value < HW_AD_KEYPAD_THRESH_FEED) return LV_KEY_ESC;
    return 0x80;
}

bool ui_keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint32_t now = esp_timer_get_time() / 1000;
    if (now - last_key_time < HW_I2C_TIMEOUT_MS) {
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

esp_err_t driver_ad_keypad_lvgl_init(void)
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
