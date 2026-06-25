#include "ui_touch.h"
#include "pin_map.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui_touch";
static spi_device_handle_t touch_spi = NULL;

static uint16_t xpt2046_read_raw(uint8_t cmd)
{
    uint8_t tx_buf[3] = {cmd, 0, 0};
    uint8_t rx_buf[3] = {0};
    spi_transaction_t t = {
        .length = 24,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };
    spi_device_polling_transmit(touch_spi, &t);
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

    if (raw_x == 0 || raw_x >= 4095 || raw_y == 0 || raw_y >= 4095) {
        prev_pressed = false;
        debounce_count = 0;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    int new_x = (int)((4095 - raw_x) * 480 / 4096);
    int new_y = (int)((4095 - raw_y) * 320 / 4096);

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
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    return false;
}

esp_err_t ui_touch_init(void)
{
    ESP_LOGI(TAG, "Initializing touch");

    gpio_set_direction(PIN_TOUCH_IRQ_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_TOUCH_IRQ_GPIO, GPIO_PULLUP_ONLY);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 2 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_TOUCH_CS_GPIO,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &touch_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Touch SPI device add failed");
        return ret;
    }

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = ui_touch_read;
    lv_indev_drv_register(&indev_drv);

    ESP_LOGI(TAG, "Touch initialized");
    return ESP_OK;
}
