// @requirement RF-RTM-003 TC-CB-001 Transições OPEN → HALF_OPEN → CLOSED
#include "unity.h"
#include "circuit_breaker.h"

static int64_t s_fake_us = 0;

int64_t esp_timer_get_time(void)
{
    return s_fake_us;
}

#include "core/circuit_breaker.c"

void setUp(void)
{
    s_fake_us = 0;
    circuit_breaker_init();
}

TEST_CASE("CB permanece CLOSED com falhas abaixo do limiar", "[circuit_breaker]")
{
    setUp();
    for (int i = 0; i < 4; i++) {
        circuit_breaker_record_failure(CB_BUS_I2C);
    }
    TEST_ASSERT_EQUAL(CB_STATE_CLOSED, circuit_breaker_get_state(CB_BUS_I2C));
    TEST_ASSERT_TRUE(circuit_breaker_is_available(CB_BUS_I2C));
}

TEST_CASE("CB abre apos 5 falhas consecutivas", "[circuit_breaker]")
{
    setUp();
    for (int i = 0; i < 5; i++) {
        circuit_breaker_record_failure(CB_BUS_I2C);
    }
    TEST_ASSERT_EQUAL(CB_STATE_OPEN, circuit_breaker_get_state(CB_BUS_I2C));
    TEST_ASSERT_FALSE(circuit_breaker_is_available(CB_BUS_I2C));
}

TEST_CASE("CB OPEN transita para HALF_OPEN apos timeout", "[circuit_breaker]")
{
    setUp();
    for (int i = 0; i < 5; i++) {
        circuit_breaker_record_failure(CB_BUS_SPI_ADC);
    }
    TEST_ASSERT_EQUAL(CB_STATE_OPEN, circuit_breaker_get_state(CB_BUS_SPI_ADC));

    s_fake_us = 31ULL * 1000000ULL;
    circuit_breaker_update();
    TEST_ASSERT_EQUAL(CB_STATE_HALF_OPEN, circuit_breaker_get_state(CB_BUS_SPI_ADC));
    TEST_ASSERT_TRUE(circuit_breaker_is_available(CB_BUS_SPI_ADC));
}

TEST_CASE("CB HALF_OPEN fecha apos sucessos consecutivos", "[circuit_breaker]")
{
    setUp();
    for (int i = 0; i < 5; i++) {
        circuit_breaker_record_failure(CB_BUS_SPI_SD);
    }
    s_fake_us = 31ULL * 1000000ULL;
    circuit_breaker_update();
    TEST_ASSERT_EQUAL(CB_STATE_HALF_OPEN, circuit_breaker_get_state(CB_BUS_SPI_SD));

    for (int i = 0; i < 3; i++) {
        circuit_breaker_record_success(CB_BUS_SPI_SD);
    }
    TEST_ASSERT_EQUAL(CB_STATE_CLOSED, circuit_breaker_get_state(CB_BUS_SPI_SD));
}

TEST_CASE("CB reset forca CLOSED", "[circuit_breaker]")
{
    setUp();
    for (int i = 0; i < 5; i++) {
        circuit_breaker_record_failure(CB_BUS_I2C);
    }
    circuit_breaker_reset(CB_BUS_I2C);
    TEST_ASSERT_EQUAL(CB_STATE_CLOSED, circuit_breaker_get_state(CB_BUS_I2C));
}
