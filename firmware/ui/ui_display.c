#include "ui_display.h"
#include "pin_map.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui_display";
static spi_device_handle_t spi_dev = NULL;

static void ili9488_send_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(PIN_TFT_DC_GPIO, 0);
    spi_device_polling_transmit(spi_dev, &t);
}

static void ili9488_send_data(const uint8_t *data, int len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    gpio_set_level(PIN_TFT_DC_GPIO, 1);
    spi_device_polling_transmit(spi_dev, &t);
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

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
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
        .tx_buffer = color_p,
    };
    spi_device_polling_transmit(spi_dev, &t);

    lv_disp_flush_ready(drv);
}

static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

esp_err_t ui_display_init(void)
{
    ESP_LOGI(TAG, "Initializing display");

    gpio_set_direction(PIN_TFT_DC_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_TFT_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_TFT_BL_GPIO, GPIO_MODE_OUTPUT);

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_SPI_MOSI_GPIO,
        .miso_io_num = PIN_SPI_MISO_GPIO,
        .sclk_io_num = PIN_SPI_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISP_BUF_SIZE * 2 + 8,
    };
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "SPI bus init failed");
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_TFT_CS_GPIO,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed");
        return ret;
    }

    ili9488_init();

    gpio_set_level(PIN_TFT_BL_GPIO, 1);

    lv_init();

    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = spi_dev;
    lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Display initialized");
    return ESP_OK;
}
