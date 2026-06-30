// @requirement RF-FLOW-BOOT-003 Self-test de subsistemas no boot
#ifndef FIRMWARE_SERVICES_SELF_TEST_H
#define FIRMWARE_SERVICES_SELF_TEST_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SELFTEST_ID_I2C_MCP23017 = 0,
    SELFTEST_ID_SPI_MCP3208,
    SELFTEST_ID_SPI_DISPLAY,
    SELFTEST_ID_SPI_SD,
    SELFTEST_ID_RELAY_P01,
    SELFTEST_ID_RELAY_P02,
    SELFTEST_ID_RELAY_P03,
    SELFTEST_ID_RELAY_P04,
    SELFTEST_ID_RELAY_P05,
    SELFTEST_ID_RELAY_P06,
    SELFTEST_ID_RELAY_P07,
    SELFTEST_ID_RELAY_P08,
    SELFTEST_ID_RELAY_P09,
    SELFTEST_ID_RELAY_P10,
    SELFTEST_ID_ACS712,
    SELFTEST_ID_DS18B20,
    SELFTEST_ID_ATO_SENSOR,
    SELFTEST_ID_AD_KEYPAD,
    SELFTEST_ID_PZEM,
    SELFTEST_ID_TOUCH,
    SELFTEST_ID_COUNT
} selftest_id_t;

/* @requirement RF-FLOW-SELFTEST-002/003 Estado por teste (não apenas booleano). */
typedef enum {
    SELFTEST_STATUS_NOT_RUN = 0,
    SELFTEST_STATUS_PASS,
    SELFTEST_STATUS_FAIL,
    SELFTEST_STATUS_SKIPPED
} selftest_status_t;

typedef struct {
    selftest_id_t id;
    const char *name;
    bool passed;
    selftest_status_t status;
    bool critical;
    uint32_t fail_count;
    char detail[64];
} selftest_result_t;

const char *self_test_status_str(selftest_status_t s);

esp_err_t self_test_init(void);
esp_err_t self_test_run_all(uint32_t timeout_ms);
esp_err_t self_test_run_one(selftest_id_t id);
const selftest_result_t* self_test_get_result(selftest_id_t id);
bool self_test_all_passed(void);
bool self_test_critical_passed(void);
uint32_t self_test_fail_count(void);
void self_test_log_results(void);

#endif
