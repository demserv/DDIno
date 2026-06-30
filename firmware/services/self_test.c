// @requirement RF-FLOW-BOOT-003 Self-test obrigatório no boot
// @requirement RF-UI-DIAG-001 Diagnóstico de subsistemas
#include "self_test.h"
#include "hardware_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "pin_map.h"
#include "hal/hal_bus.h"
#include "hardware_config.h"
#include "driver/gpio.h"
#include "driver_mcp3208.h"
#include "driver_acs712.h"
#include "driver_pzem.h"
#include "driver_relay.h"
#include "storage_sd.h"
#include "driver_ds18b20.h"
#include "driver_ili9488.h"
#include "driver_xpt2046.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "services/wdt_stats.h"
#include "driver_ds3231.h"
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
    [SELFTEST_ID_NVS]          = "NVS",
    [SELFTEST_ID_RTC]          = "RTC_DS3231",
    [SELFTEST_ID_HEAP]         = "HEAP",
    [SELFTEST_ID_WDT]          = "WDT",
    [SELFTEST_ID_MCP3208_CH2]  = "MCP3208_CH2",
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
    [SELFTEST_ID_NVS]          = true,
    [SELFTEST_ID_RTC]          = false,
    [SELFTEST_ID_HEAP]         = true,
    [SELFTEST_ID_WDT]          = false,
    [SELFTEST_ID_MCP3208_CH2]  = true,
};

esp_err_t self_test_init(void)
{
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        s_results[i].id = (selftest_id_t)i;
        s_results[i].name = s_names[i];
        s_results[i].passed = false;
        s_results[i].status = SELFTEST_STATUS_NOT_RUN;
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
    esp_err_t err = hal_i2c_master_write_read_device(I2C_NUM_0, 0x20, &reg, 1, &val, 1, pdMS_TO_TICKS(HW_I2C_TIMEOUT_MS));
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
    if (!driver_ili9488_is_ok()) {
        r->status = SELFTEST_STATUS_SKIPPED;
        snprintf(r->detail, sizeof(r->detail), "display nao inicializado");
        return;
    }
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

/* @requirement RF-FLOW-BOOT-003 Self-test do caminho de controle de relés.
 * Não energiza cargas: verifica a disponibilidade do caminho de atuação.
 * P03..P10 dependem do módulo MCP23017; P01/P02 são GPIO direto. */
static void test_relay(selftest_result_t *r, selftest_id_t id)
{
    if (id >= SELFTEST_ID_RELAY_P03 && id <= SELFTEST_ID_RELAY_P10) {
        if (!relay_mcp23017_ok()) {
            r->fail_count++;
            snprintf(r->detail, sizeof(r->detail), "MCP23017 relay module not detected");
            return;
        }
        uint8_t gpioa = 0xFF;
        if (mcp23017_read_port(&gpioa, NULL) != ESP_OK || gpioa != 0x00) {
            r->fail_count++;
            snprintf(r->detail, sizeof(r->detail), "MCP23017 readback=0x%02X (esperado 0x00)", gpioa);
            return;
        }
        r->passed = true;
        return;
    }

    int pin = (id == SELFTEST_ID_RELAY_P01) ? PIN_RELAY_P01_GPIO : PIN_RELAY_P02_GPIO;
    int level = gpio_get_level(pin);
    int expected = HW_RELAY_SAFE_OFF_LEVEL;
    if (level == expected) {
        r->passed = true;
        snprintf(r->detail, sizeof(r->detail), "GPIO=%d OK (OFF)", level);
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "GPIO=%d esperado %d", level, expected);
    }
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
    if (!driver_xpt2046_is_ok()) {
        r->status = SELFTEST_STATUS_SKIPPED;
        snprintf(r->detail, sizeof(r->detail), "touch nao inicializado");
        return;
    }
    r->passed = true;
}

static void test_nvs(selftest_result_t *r)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("selftest", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        nvs_close(nvs);
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "nvs open %s", esp_err_to_name(err));
    }
}

static void test_rtc(selftest_result_t *r)
{
    ds3231_time_t rt = {0};
    if (ds3231_get_time(&rt) == ESP_OK && rt.year >= 2020) {
        r->passed = true;
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "rtc read fail");
    }
}

static void test_heap(selftest_result_t *r)
{
    size_t free_heap = esp_get_free_heap_size();
    if (free_heap >= HW_HEAP_MIN_BYTES) {
        r->passed = true;
        snprintf(r->detail, sizeof(r->detail), "free=%u", (unsigned)free_heap);
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "free=%u min=%u",
                 (unsigned)free_heap, (unsigned)HW_HEAP_MIN_BYTES);
    }
}

static void test_wdt(selftest_result_t *r)
{
    uint32_t resets = wdt_stats_get_resets_24h();
    snprintf(r->detail, sizeof(r->detail), "resets24h=%lu", (unsigned long)resets);
    if (resets <= HW_WDT_RESET_MAX_24H) {
        r->passed = true;
    } else {
        r->fail_count++;
    }
}

static void test_mcp3208_ch2(selftest_result_t *r)
{
    uint16_t adc_val = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_P03_ACS, &adc_val);
    if (err == ESP_OK) {
        r->passed = true;
        snprintf(r->detail, sizeof(r->detail), "adc=%u", (unsigned)adc_val);
    } else {
        r->fail_count++;
        snprintf(r->detail, sizeof(r->detail), "err %s", esp_err_to_name(err));
    }
}

esp_err_t self_test_run_one(selftest_id_t id)
{
    if (id >= SELFTEST_ID_COUNT) return ESP_ERR_INVALID_ARG;

    selftest_result_t *r = &s_results[id];
    r->passed = false;
    r->status = SELFTEST_STATUS_NOT_RUN;
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
    case SELFTEST_ID_RELAY_P10:    test_relay(r, id);     break;
    case SELFTEST_ID_ACS712:       test_acs712(r);        break;
    case SELFTEST_ID_DS18B20:      test_ds18b20(r);       break;
    case SELFTEST_ID_ATO_SENSOR:   test_ato_sensor(r);    break;
    case SELFTEST_ID_AD_KEYPAD:    test_ad_keypad(r);     break;
    case SELFTEST_ID_PZEM:         test_pzem(r);          break;
    case SELFTEST_ID_TOUCH:        test_touch(r);         break;
    case SELFTEST_ID_NVS:          test_nvs(r);           break;
    case SELFTEST_ID_RTC:          test_rtc(r);           break;
    case SELFTEST_ID_HEAP:         test_heap(r);          break;
    case SELFTEST_ID_WDT:          test_wdt(r);           break;
    case SELFTEST_ID_MCP3208_CH2:  test_mcp3208_ch2(r);   break;
    default: break;
    }

    /* Deriva o status final; testes que não puderam ser verificados já marcaram SKIPPED. */
    if (r->status == SELFTEST_STATUS_SKIPPED) {
        if (!r->critical) {
            r->passed = true;
        }
    } else {
        r->status = r->passed ? SELFTEST_STATUS_PASS : SELFTEST_STATUS_FAIL;
    }

    ESP_LOGI(TAG, "%s: %s", r->name, self_test_status_str(r->status));
    return ESP_OK;
}

const char *self_test_status_str(selftest_status_t s)
{
    switch (s) {
        case SELFTEST_STATUS_PASS:    return "PASS";
        case SELFTEST_STATUS_FAIL:    return "FAIL";
        case SELFTEST_STATUS_SKIPPED: return "SKIPPED";
        case SELFTEST_STATUS_NOT_RUN:
        default:                      return "NOT_RUN";
    }
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
        if (s_results[i].status == SELFTEST_STATUS_SKIPPED) continue;
        if (!s_results[i].passed) count++;
    }
    return count;
}

void self_test_log_results(void)
{
    ESP_LOGI(TAG, "===== SELF-TEST RESULTS =====");
    for (int i = 0; i < SELFTEST_ID_COUNT; i++) {
        const selftest_result_t *r = &s_results[i];
        ESP_LOGI(TAG, "[%s] %s: %s", r->critical ? "CRIT" : "INFO", r->name,
                 self_test_status_str(r->status));
        if (!r->passed && r->detail[0]) {
            ESP_LOGI(TAG, "       %s", r->detail);
        }
    }
    uint32_t passed_count = SELFTEST_ID_COUNT - self_test_fail_count();
    ESP_LOGI(TAG, "Passed: %lu/%d, Critical OK: %s",
             (unsigned long)passed_count, SELFTEST_ID_COUNT,
             self_test_critical_passed() ? "YES" : "NO");
}
