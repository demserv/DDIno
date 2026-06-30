/* @requirement RF-DISP-TOUCH-001 a RF-DISP-TOUCH-005 */
#include "driver_xpt2046.h"
#include "pin_map.h"
#include "hal_spi.h"
#include "hardware_config.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_screen_manager.h"

#define TOUCH_ADC_MAX    4095
#define TOUCH_ADC_RANGE  4096

static const char *TAG = "driver_xpt2046";
static bool s_touch_init_ok = false;

static uint16_t xpt2046_read_raw(uint8_t cmd)
{
    uint8_t tx_buf[3] = {cmd, 0, 0};
    uint8_t rx_buf[3] = {0};
    spi_transaction_t t = {
        .length = 24,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };
    hal_spi_transaction_polling(HAL_SPI_DEVICE_TOUCH, &t);
    return (((uint16_t)rx_buf[1] << 8) | rx_buf[2]) >> 4;
}

static int prev_x = -1;
static int prev_y = -1;
static bool prev_pressed = false;
static int stable_x = -1;
static int stable_y = -1;
static bool stable_pressed = false;
static int debounce_count = 0;

bool ui_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    if (gpio_get_level(PIN_TOUCH_IRQ_GPIO) != 0) {
        prev_pressed = false;
        debounce_count = 0;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    uint16_t raw_x = xpt2046_read_raw(0x90);
    uint16_t raw_y = xpt2046_read_raw(0xD0);

    if (raw_x == 0 || raw_x >= TOUCH_ADC_MAX || raw_y == 0 || raw_y >= TOUCH_ADC_MAX) {
        prev_pressed = false;
        debounce_count = 0;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    int new_x = (int)((TOUCH_ADC_MAX - raw_x) * 480 / TOUCH_ADC_RANGE);
    int new_y = (int)((TOUCH_ADC_MAX - raw_y) * 320 / TOUCH_ADC_RANGE);

    if (new_x < 0) new_x = 0;
    if (new_x > 479) new_x = 479;
    if (new_y < 0) new_y = 0;
    if (new_y > 319) new_y = 319;

    if (new_x == prev_x && new_y == prev_y) {
        debounce_count++;
    } else {
        debounce_count = 0;
    }

    prev_x = new_x;
    prev_y = new_y;

    if (debounce_count >= 2) {
        stable_x = new_x;
        stable_y = new_y;
        stable_pressed = true;
        data->state = LV_INDEV_STATE_PR;
        data->point.x = stable_x;
        data->point.y = stable_y;
        ui_screen_manager_on_user_interaction();
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    return false;
}

esp_err_t driver_xpt2046_init(void)
{
    ESP_LOGI(TAG, "Initializing touch");

    gpio_set_direction(PIN_TOUCH_IRQ_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_TOUCH_IRQ_GPIO, GPIO_PULLUP_ONLY);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = ui_touch_read;
    lv_indev_drv_register(&indev_drv);

    /* @requirement RF-DISP-TOUCH-001 Probe do barramento: uma leitura SPI do XPT2046
     * com IRQ não-pressionado deve produzir um código ADC plausível (<= 12 bits). */
    uint16_t probe = xpt2046_read_raw(0x90);
    s_touch_init_ok = (probe <= TOUCH_ADC_MAX);

    ESP_LOGI(TAG, "Touch initialized (probe=%u)", (unsigned)probe);
    return ESP_OK;
}

bool driver_xpt2046_is_ok(void)
{
    return s_touch_init_ok;
}
