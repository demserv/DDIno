// @requirement RF-TIME-001 DS3231 RTC como fonte de tempo principal
// @requirement RNF-HARDWARE-001 I2C RTC com bateria CR2032
#include "driver_ds3231.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "ds3231";

#define DS3231_ADDR      0x68
#define DS3231_REG_SEC   0x00
#define DS3231_REG_TEMP  0x11

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

esp_err_t ds3231_init(void)
{
    ESP_LOGI(TAG, "DS3231 RTC initialized");
    return ESP_OK;
}

esp_err_t ds3231_set_time(const ds3231_time_t *t)
{
    uint8_t month_reg = dec_to_bcd(t->month) & 0x1F;
    if (t->year < 2000) {
        month_reg |= 0x20;
    }
    uint8_t buf[8] = {
        DS3231_REG_SEC,
        dec_to_bcd(t->second) & 0x7F,
        dec_to_bcd(t->minute),
        dec_to_bcd(t->hour) & 0x3F,
        0x01,
        dec_to_bcd(t->day),
        month_reg,
        dec_to_bcd(t->year % 100)
    };
    return i2c_master_write_to_device(I2C_NUM_0, DS3231_ADDR, buf, 8, pdMS_TO_TICKS(100));
}

esp_err_t ds3231_get_time(ds3231_time_t *t)
{
    uint8_t reg = DS3231_REG_SEC;
    uint8_t buf[7];
    esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, DS3231_ADDR, &reg, 1, buf, 7, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;

    t->second = bcd_to_dec(buf[0] & 0x7F);
    t->minute = bcd_to_dec(buf[1]);

    uint8_t hour_reg = buf[2];
    if (hour_reg & 0x40) {
        uint8_t h12 = hour_reg & 0x1F;
        bool pm = (hour_reg & 0x20) != 0;
        t->hour = bcd_to_dec(h12);
        if (pm) {
            if (t->hour != 12) t->hour += 12;
        } else {
            if (t->hour == 12) t->hour = 0;
        }
    } else {
        t->hour = bcd_to_dec(hour_reg & 0x3F);
    }

    t->day = bcd_to_dec(buf[4]);
    uint8_t month_reg = buf[5];
    t->month = bcd_to_dec(month_reg & 0x1F);
    if (month_reg & 0x20) {
        t->year = 1900 + bcd_to_dec(buf[6]);
    } else {
        t->year = 2000 + bcd_to_dec(buf[6]);
    }

    return ESP_OK;
}

bool ds3231_is_running(void)
{
    uint8_t reg = DS3231_REG_SEC;
    uint8_t val = 0;
    esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, DS3231_ADDR, &reg, 1, &val, 1, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return false;
    return (val & 0x80) == 0;
}

float ds3231_get_temp(void)
{
    uint8_t reg = DS3231_REG_TEMP;
    uint8_t buf[2];
    esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, DS3231_ADDR, &reg, 1, buf, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return 0.0f;
    float temp = (int8_t)buf[0];
    uint8_t frac = (buf[1] >> 6) & 0x03;
    temp += frac * 0.25f;
    return temp;
}
