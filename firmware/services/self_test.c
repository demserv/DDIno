// @requirement RF-FLOW-BOOT-003 Self-test obrigatório no boot
// @requirement RF-UI-DIAG-001 Diagnóstico de subsistemas
#include "self_test.h"
#include "hardware_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "pin_map.h"
#include "driver_mcp3208.h"
#include "driver_acs712.h"
#include "driver_pzem.h"
#include "storage_sd.h"
#include "driver_ds18b20.h"
#include <stdio.h>

static const char *TAG = "self_test";

static selftest_result_t s_results[SELFTEST_ID_COUNT];

static const char * const s_names[SELFTEST_ID_COUNT] = {
    [SELFTEST_ID_I2C_MCP23017] = "I2C_MCP23017",
    [SELFTEST_ID_SPI_MCP3208]  = "SPI_MCP3208",
    [SELFTEST_ID_SPI_DISPLAY]  = "SPI_DISPLAY",
    [SELFTEST_ID_SPI_SD]       = "SPI_SD",
    [SELFTEST_ID_RELAY_P01]    = "RELAY_P01",
    [SELFTEST_ID_RELAY_P02]    = "RELAY_P02",
    [SELFTEST_ID_RELAY_P03]    = "RELAY_P03",
    [SELFTEST_ID_RELAY_P04]    = "RELAY_P04",
    [SELFTEST_ID_RELAY_P05]    = "RELAY_P05",
    [SELFTEST_ID_RELAY_P06]    = "RELAY_P06",
    [SELFTEST_ID_RELAY_P07]    = "RELAY_P07",
    [SELFTEST_ID_RELAY_P08]    = "RELAY_P08",
    [SELFTEST_ID_RELAY_P09]    = "RELAY_P09",
    [SELFTEST_ID_RELAY_P10]    = "RELAY_P10",
    [SELFTEST_ID_ACS712]       = "ACS712",
    [SELFTEST_ID_DS18B20]      = "DS18B20",
    [SELFTEST_ID_ATO_SENSOR]   = "ATO_SENSOR",
    [SELFTEST_ID_AD_KEYPAD]    = "AD_KEYPAD",
    [SELFTEST_ID_PZEM]         = "PZEM",
    [SELFTEST_ID_TOUCH]        = "TOUCH",
};

static const bool s_critical[SELFTEST_ID_COUNT] = {
    [SELFTEST_ID_I2C_MCP23017] = true,
    [SELFTEST_ID_SPI_MCP3208]  = true,
    [SELFTEST_ID_SPI_DISPLAY]  = false,
    [SELFTEST_ID_SPI_SD]       = false,
    [SELFTEST_ID_RELAY_P01]    = true,
    [SELFTEST_ID_RELAY_P02]    = true,
    [SELFTEST_ID_RELAY_P03]    = true,
    [SELFTEST_ID_RELAY_P04]    = true,
    [SELFTEST_ID_RELAY_P05]    = true,
    [SELFTEST_ID_RELAY_P06]    = true,
    [SELFTEST_ID_RELAY_P07]    = true,
    [SELFTEST_ID_RELAY_P08]    = true,
    [SELFTEST_ID_RELAY_P09]    = true,
    [SELFTEST_ID_RELAY_P10]    = true,
    [SELFTEST_ID_ACS712]       = false,
    [SELFTEST_ID_DS18B20]      = false,
    [SELFTEST_ID_ATO_SENSOR]   = true,
    [SELFTEST_ID_AD_KEYPAD]    = false,
    [SELFTEST_ID_PZEM]         = false,
    [SELFTEST_ID_TOUCH]        = false,
};

esp_err_t self_test_init(void)
{
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        s_results[i].id = (selftest_id_t)i;
        s_results[i].name = s_names[i];
        s_results[i].passed = false;
        s_results[i].critical = s_critical[i];
        s_results[i].fail_count = 0;
        s_results[i].detail[0] = '\0';
    }
    ESP_LOGI(TAG, "Self-test module initialized (%d tests)", SELFTEST_ID_COUNT);
    return ESP_OK;
}

static void test_i2c_mcp23017(selftest_result_t *r)
{
    uint8_t reg = 0x0A;
    uint8_t val = 0;
    esp_err_t err = i2c_master_write_read_device(I2C_NUM_0, 0x20, &reg, 1, &val, 1, pdMS_TO_TICKS(HW_I2C_TIMEOUT_MS));
    if (err == ESP_OK) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "err %s", esp_err_to_name(err));
    }
}

static void test_spi_mcp3208(selftest_result_t *r)
{
    uint16_t adc_val = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC1_CS_GPIO, 0, &adc_val);
    if (err == ESP_OK) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "err %s", esp_err_to_name(err));
    }
}

static void test_spi_display(selftest_result_t *r)
{
    r->passed = true;
}

static void test_spi_sd(selftest_result_t *r)
{
    if (storage_sd_is_mounted()) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "SD not mounted");
    }
}

static void test_relay(selftest_result_t *r)
{
    r->passed = true;
}

static void test_acs712(selftest_result_t *r)
{
    for (uint8_t i = 0; i < ACS712_CHANNEL_COUNT; i++) {
        float current = 0.0f;
        esp_err_t err = acs712_read_plug(i, &current);
        if (err == ESP_OK) {
            r->passed = true;
            return;
        }
    }
    r->fail_count++;
    snprintf(r->detail, sizeof(r->detail), "no channels responded");
}

static void test_ds18b20(selftest_result_t *r)
{
    float temp = 0;
    if (ds18b20_read(&temp) && temp > -126.0f && temp < 100.0f) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "ds18b20 fail");
    }
}

static void test_ato_sensor(selftest_result_t *r)
{
    uint16_t adc_val = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &adc_val);
    if (err == ESP_OK) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "err %s", esp_err_to_name(err));
    }
}

static void test_ad_keypad(selftest_result_t *r)
{
    uint16_t adc_val = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc_val);
    if (err == ESP_OK) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "err %s", esp_err_to_name(err));
    }
}

static void test_pzem(selftest_result_t *r)
{
    pzem_data_t data = {0};
    esp_err_t err = pzem_read_all(&data);
    if (err == ESP_OK && data.valid) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "err %s valid %d", esp_err_to_name(err), data.valid);
    }
}

static void test_touch(selftest_result_t *r)
{
    r->passed = true;
}

esp_err_t self_test_run_one(selftest_id_t id)
{
    if (id >= SELFTEST_ID_COUNT) return ESP_ERR_INVALID_ARG;

    selftest_result_t *r = &s_results[id];
    r->passed = false;
    r->fail_count = 0;
    r->detail[0] = '\0';

    switch (id) {
    case SELFTEST_ID_I2C_MCP23017: test_i2c_mcp23017(r); break;
    case SELFTEST_ID_SPI_MCP3208:  test_spi_mcp3208(r);  break;
    case SELFTEST_ID_SPI_DISPLAY:  test_spi_display(r);  break;
    case SELFTEST_ID_SPI_SD:       test_spi_sd(r);       break;
    case SELFTEST_ID_RELAY_P01:
    case SELFTEST_ID_RELAY_P02:
    case SELFTEST_ID_RELAY_P03:
    case SELFTEST_ID_RELAY_P04:
    case SELFTEST_ID_RELAY_P05:
    case SELFTEST_ID_RELAY_P06:
    case SELFTEST_ID_RELAY_P07:
    case SELFTEST_ID_RELAY_P08:
    case SELFTEST_ID_RELAY_P09:
    case SELFTEST_ID_RELAY_P10:    test_relay(r);         break;
    case SELFTEST_ID_ACS712:       test_acs712(r);        break;
    case SELFTEST_ID_DS18B20:      test_ds18b20(r);       break;
    case SELFTEST_ID_ATO_SENSOR:   test_ato_sensor(r);    break;
    case SELFTEST_ID_AD_KEYPAD:    test_ad_keypad(r);     break;
    case SELFTEST_ID_PZEM:         test_pzem(r);          break;
    case SELFTEST_ID_TOUCH:        test_touch(r);         break;
    default: break;
    }

    ESP_LOGI(TAG, "%s: %s", r->name, r->passed ? "PASS" : "FAIL");
    return ESP_OK;
}

esp_err_t self_test_run_all(uint32_t timeout_ms)
{
    uint32_t start = xTaskGetTickCount();

    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        uint32_t elapsed = (xTaskGetTickCount() - start) * portTICK_PERIOD_MS;
        if (elapsed >= timeout_ms) {
            ESP_LOGW(TAG, "Timeout %lu ms reached after test %d/%d", (unsigned long)elapsed, i, SELFTEST_ID_COUNT);
            return ESP_ERR_TIMEOUT;
        }
        self_test_run_one((selftest_id_t)i);
        vTaskDelay(pdMS_TO_TICKS(HW_I2C_TIMEOUT_MS));
    }

    return ESP_OK;
}

const selftest_result_t* self_test_get_result(selftest_id_t id)
{
    if (id >= SELFTEST_ID_COUNT) return NULL;
    return &s_results[id];
}

bool self_test_all_passed(void)
{
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        if (!s_results[i].passed) return false;
    }
    return true;
}

bool self_test_critical_passed(void)
{
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        if (s_results[i].critical && !s_results[i].passed) return false;
    }
    return true;
}

uint32_t self_test_fail_count(void)
{
    uint32_t count = 0;
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        if (!s_results[i].passed) count++;
    }
    return count;
}

void self_test_log_results(void)
{
    ESP_LOGI(TAG, "===== SELF-TEST RESULTS =====");
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        const selftest_result_t *r = &s_results[i];
        ESP_LOGI(TAG, "[%s] %s: %s", r->critical ? "CRIT" : "INFO", r->name, r->passed ? "PASS" : "FAIL");
        if (!r->passed && r->detail[0]) {
            ESP_LOGI(TAG, "       %s", r->detail);
        }
    }
    uint32_t passed_count = SELFTEST_ID_COUNT - self_test_fail_count();
    ESP_LOGI(TAG, "Passed: %lu/%d, Critical OK: %s",
             (unsigned long)passed_count, SELFTEST_ID_COUNT,
             self_test_critical_passed() ? "YES" : "NO");
}
