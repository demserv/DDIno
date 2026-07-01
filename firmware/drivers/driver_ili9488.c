/* @requirement RF-DISP-001 a RF-DISP-035 init ILI9488 */
#include "driver_ili9488.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui/hmi/ui_theme.h"

#include "hal_spi.h"
#include "hardware_config.h"
#include "pin_map.h"
#include "circuit_breaker.h"

static const char *TAG = "driver_ili9488";

static uint8_t s_brightness_percent = HW_UI_BRIGHTNESS_DEFAULT;
static uint8_t s_configured_brightness = HW_UI_BRIGHTNESS_DEFAULT;
static bool s_display_init_ok = false;
static bool s_dim_enabled = false;
static uint32_t s_dim_timeout_s = 0;

esp_err_t driver_ili9488_backlight_init(int gpio_num);
esp_err_t driver_ili9488_backlight_set(uint8_t percent);

static esp_err_t ili9488_spi_tx(spi_transaction_t *t)
{
    if (!circuit_breaker_is_available(CB_BUS_SPI_DISPLAY)) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = hal_spi_transaction_polling(HAL_SPI_DEVICE_TFT, t);
    if (err == ESP_OK) {
        circuit_breaker_record_success(CB_BUS_SPI_DISPLAY);
    } else {
        circuit_breaker_record_failure(CB_BUS_SPI_DISPLAY);
    }
    return err;
}

static void ili9488_send_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(PIN_TFT_DC_GPIO, 0);
    (void)ili9488_spi_tx(&t);
}

static void ili9488_send_data(const uint8_t *data, int len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    gpio_set_level(PIN_TFT_DC_GPIO, 1);
    (void)ili9488_spi_tx(&t);
}

static void ili9488_send_cmd_data(uint8_t cmd, uint8_t data)
{
    ili9488_send_cmd(cmd);
    ili9488_send_data(&data, 1);
}

static void ili9488_init(void)
{
    gpio_set_level(PIN_TFT_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(HW_TFT_RESET_PULSE_MS));
    gpio_set_level(PIN_TFT_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(HW_TFT_INIT_DELAY_MS));

    ili9488_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(HW_TFT_INIT_DELAY_MS));

    ili9488_send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(HW_TFT_INIT_DELAY_MS));

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

esp_err_t driver_ili9488_init(void)
{
    ESP_LOGI(TAG, "Initializing ILI9488 display");

    gpio_set_direction(PIN_TFT_DC_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_TFT_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_TFT_BL_GPIO, GPIO_MODE_OUTPUT);

    ili9488_init();

    ESP_ERROR_CHECK(driver_ili9488_backlight_init(PIN_TFT_BL_GPIO));
    driver_ili9488_backlight_set(s_brightness_percent);

    lv_init();

    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = HW_DISP_HOR_RES;
    disp_drv.ver_res = HW_DISP_VER_RES;
    disp_drv.flush_cb = ui_display_lvgl_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * USEC_PER_MSEC));

    s_display_init_ok = true;
    circuit_breaker_record_success(CB_BUS_SPI_DISPLAY);
    ESP_LOGI(TAG, "ILI9488 display initialized");
    return ESP_OK;
}

bool driver_ili9488_is_ok(void)
{
    return s_display_init_ok;
}

void ui_display_set_brightness(uint8_t percent)
{
    if (percent > HW_DISP_BRIGHTNESS_MAX_PCT) {
        percent = HW_DISP_BRIGHTNESS_MAX_PCT;
    }
    s_configured_brightness = percent;
    s_brightness_percent = percent;
    driver_ili9488_backlight_set(percent);
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
        driver_ili9488_backlight_set(s_brightness_percent);
    }
}

/* @requirement RF-DISP-BACKLIGHT-001 PWM via LEDC */
#define DISP_BL_LEDC_MODE   LEDC_LOW_SPEED_MODE
#define DISP_BL_LEDC_TIMER  LEDC_TIMER_0
#define DISP_BL_LEDC_CH     LEDC_CHANNEL_0
#define DISP_BL_LEDC_FREQ   5000
#define DISP_BL_LEDC_RES    LEDC_TIMER_13_BIT
#define DISP_BL_LEDC_DUTY_MAX 8191

esp_err_t driver_ili9488_backlight_init(int gpio_num)
{
    ledc_timer_config_t t = {
        .speed_mode      = DISP_BL_LEDC_MODE,
        .timer_num       = DISP_BL_LEDC_TIMER,
        .duty_resolution = DISP_BL_LEDC_RES,
        .freq_hz         = DISP_BL_LEDC_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&t);
    if (err != ESP_OK) return err;
    ledc_channel_config_t c = {
        .gpio_num   = gpio_num,
        .speed_mode = DISP_BL_LEDC_MODE,
        .channel    = DISP_BL_LEDC_CH,
        .timer_sel  = DISP_BL_LEDC_TIMER,
        .duty       = DISP_BL_LEDC_DUTY_MAX,
        .hpoint     = 0
    };
    return ledc_channel_config(&c);
}

esp_err_t driver_ili9488_backlight_set(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint32_t duty = (uint32_t)((DISP_BL_LEDC_DUTY_MAX * (uint32_t)percent) / 100U);
    esp_err_t err = ledc_set_duty(DISP_BL_LEDC_MODE, DISP_BL_LEDC_CH, duty);
    if (err != ESP_OK) return err;
    return ledc_update_duty(DISP_BL_LEDC_MODE, DISP_BL_LEDC_CH);
}

esp_err_t ui_display_set_backlight(bool enabled)
{
    ui_display_set_brightness(enabled ? HW_DISP_BRIGHTNESS_MAX_PCT : 0U);
    return ESP_OK;
}

