#include "ui_display.h"
#include "hardware_config.h"
#include "pin_map.h"
#include "hal_spi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui_display";

static uint8_t s_brightness_percent = HW_UI_BRIGHTNESS_DEFAULT;
static uint8_t s_configured_brightness = HW_UI_BRIGHTNESS_DEFAULT;
static bool s_dim_enabled = false;
static uint32_t s_dim_timeout_s = 0;

static void ili9488_send_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(PIN_TFT_DC_GPIO, 0);
    hal_spi_transaction_polling(HAL_SPI_DEVICE_TFT, &t);
}

static void ili9488_send_data(const uint8_t *data, int len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    gpio_set_level(PIN_TFT_DC_GPIO, 1);
    hal_spi_transaction_polling(HAL_SPI_DEVICE_TFT, &t);
}

static void ili9488_send_cmd_data(uint8_t cmd, uint8_t data)
{
    ili9488_send_cmd(cmd);
    ili9488_send_data(&data, 1);
}

static void ili9488_init(void)
{
    gpio_set_level(PIN_TFT_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_TFT_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    ili9488_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(120));

    ili9488_send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    ili9488_send_cmd_data(0x3A, 0x55);
    ili9488_send_cmd_data(0x36, 0x48);
    ili9488_send_cmd(0x29);
}

void ui_display_lvgl_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    uint8_t caset[4] = {
        (uint8_t)((area->x1 >> 8) & 0xFF),
        (uint8_t)(area->x1 & 0xFF),
        (uint8_t)((area->x2 >> 8) & 0xFF),
        (uint8_t)(area->x2 & 0xFF)
    };
    ili9488_send_cmd(0x2A);
    ili9488_send_data(caset, 4);

    uint8_t raset[4] = {
        (uint8_t)((area->y1 >> 8) & 0xFF),
        (uint8_t)(area->y1 & 0xFF),
        (uint8_t)((area->y2 >> 8) & 0xFF),
        (uint8_t)(area->y2 & 0xFF)
    };
    ili9488_send_cmd(0x2B);
    ili9488_send_data(raset, 4);

    ili9488_send_cmd(0x2C);

    int pixel_count = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    gpio_set_level(PIN_TFT_DC_GPIO, 1);
    spi_transaction_t t = {
        .length = pixel_count * 2 * 8,
        .tx_buffer = color_map,
    };
    hal_spi_transaction_polling(HAL_SPI_DEVICE_TFT, &t);

    lv_disp_flush_ready(drv);
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

esp_err_t ui_display_init(void)
{
    ESP_LOGI(TAG, "Initializing ILI9488 display");

    gpio_set_direction(PIN_TFT_DC_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_TFT_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_TFT_BL_GPIO, GPIO_MODE_OUTPUT);

    ili9488_init();

    gpio_set_level(PIN_TFT_BL_GPIO, s_brightness_percent > 0 ? 1 : 0);

    lv_init();

    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = ui_display_lvgl_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "ILI9488 display initialized");
    return ESP_OK;
}

void ui_display_set_brightness(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    s_configured_brightness = percent;
    s_brightness_percent = percent;
    gpio_set_level(PIN_TFT_BL_GPIO, percent > 0 ? 1 : 0);
}

uint8_t ui_display_get_brightness(void)
{
    return s_brightness_percent;
}

void ui_display_dim_on_inactivity(bool enable, uint32_t timeout_s)
{
    s_dim_enabled = enable;
    s_dim_timeout_s = timeout_s;
    if (!enable) {
        s_brightness_percent = s_configured_brightness;
        gpio_set_level(PIN_TFT_BL_GPIO, s_brightness_percent > 0 ? 1 : 0);
    }
}

esp_err_t ui_display_set_backlight(bool enabled)
{
    ui_display_set_brightness(enabled ? 100U : 0U);
    return ESP_OK;
}
