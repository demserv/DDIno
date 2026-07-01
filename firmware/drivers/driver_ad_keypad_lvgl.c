/* @requirement RF-UI-INPUT-001 Gestos do keypad: MUTE (UP 5s) e HOME (duplo ENTER)
 * Decisao normativa do usuario — 2026-06-30 (itens 5 e 6). */
#include "driver_ad_keypad_lvgl.h"
#include "hardware_config.h"
#include "pin_map.h"
#include "driver_mcp3208.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_screen_manager.h"

static const char *TAG = "driver_ad_keypad_lvgl";

static uint32_t last_key_time = 0;
static int last_key = LV_KEY_ENTER;
static keypad_special_cb_t s_special_cb = NULL;

static bool s_up_holding = false;
static uint32_t s_up_hold_start_ms = 0;
static bool s_mute_fired_this_hold = false;

static bool s_enter_pressed = false;
static uint32_t s_last_enter_edge_ms = 0;

void driver_ad_keypad_set_special_cb(keypad_special_cb_t cb)
{
    s_special_cb = cb;
}

static bool adc_is_up(uint16_t adc_value)
{
    return adc_value >= HW_AD_KEYPAD_ADC_UP_THRESH_MIN &&
           adc_value < HW_AD_KEYPAD_THRESH_LEFT;
}

static bool adc_is_enter(uint16_t adc_value)
{
    return adc_value >= HW_AD_KEYPAD_THRESH_RIGHT &&
           adc_value < HW_AD_KEYPAD_THRESH_MENU;
}

static int map_adc_to_lvgl_key(uint16_t adc_value)
{
    if (adc_value < HW_AD_KEYPAD_THRESH_DOWN) return 0;
    if (adc_value < HW_AD_KEYPAD_THRESH_LEFT) return LV_KEY_UP;
    if (adc_value < HW_AD_KEYPAD_THRESH_RIGHT) return LV_KEY_DOWN;
    if (adc_value < HW_AD_KEYPAD_THRESH_ENTER) return LV_KEY_LEFT;
    if (adc_value < HW_AD_KEYPAD_THRESH_ESC) return LV_KEY_RIGHT;
    if (adc_value < HW_AD_KEYPAD_THRESH_MENU) return LV_KEY_ENTER;
    /* @requirement RF-UI-SHORTCUT-001 Faixa ADC FEED (3000..3600) → atalho Feed. */
    if (adc_value < HW_AD_KEYPAD_THRESH_FEED) return LV_KEY_FEED;
    return 0;
}

void driver_ad_keypad_gesture_poll(void)
{
    uint16_t adc_value;
    if (mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc_value) != ESP_OK) {
        return;
    }

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);

    if (adc_is_up(adc_value)) {
        if (!s_up_holding) {
            s_up_holding = true;
            s_up_hold_start_ms = now;
            s_mute_fired_this_hold = false;
        } else if (!s_mute_fired_this_hold &&
                   (now - s_up_hold_start_ms) >= HW_KEYPAD_MUTE_HOLD_MS) {
            s_mute_fired_this_hold = true;
            if (s_special_cb) s_special_cb(LV_KEY_MUTE);
        }
    } else {
        s_up_holding = false;
        s_mute_fired_this_hold = false;
    }

    bool enter_now = adc_is_enter(adc_value);
    if (enter_now && !s_enter_pressed) {
        if (s_last_enter_edge_ms != 0 &&
            (now - s_last_enter_edge_ms) <= HW_KEYPAD_HOME_DOUBLE_MS) {
            if (s_special_cb) s_special_cb(LV_KEY_HOME);
            s_last_enter_edge_ms = 0;
        } else {
            s_last_enter_edge_ms = now;
        }
    }
    s_enter_pressed = enter_now;
}

void ui_keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    uint32_t now = esp_timer_get_time() / 1000;
    if (now - last_key_time < HW_I2C_TIMEOUT_MS) {
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    uint16_t adc_value;
    esp_err_t ret = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc_value);
    if (ret != ESP_OK) {
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    int key = map_adc_to_lvgl_key(adc_value);
    if (key == 0 || key == last_key) {
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    last_key = key;
    last_key_time = now;

    /* Teclas especiais (FEED/HOME/MUTE): roteadas para a UI. */
    if (key == LV_KEY_FEED || key == LV_KEY_HOME || key == LV_KEY_MUTE) {
        if (s_special_cb) s_special_cb(key);
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    data->state = LV_INDEV_STATE_PR;
    data->key = key;
    ui_screen_manager_on_user_interaction();
}

esp_err_t driver_ad_keypad_lvgl_init(void)
{
    ESP_LOGI(TAG, "Initializing AD keypad");

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = ui_keypad_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    if (!indev) {
        ESP_LOGE(TAG, "Falha ao registrar indev keypad");
        return ESP_FAIL;
    }

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(indev, group);

    ESP_LOGI(TAG, "AD keypad initialized");
    return ESP_OK;
}
