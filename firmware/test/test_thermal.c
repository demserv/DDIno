#include <string.h>
#include "unity.h"
#include "include/system_types.h"
#include "fsm/thermal_fsm.h"

/* include production source */
#include "fsm/thermal_fsm.c"

static thermal_fsm_t fsm;
static thermal_params_t cfg;

void setUp(void)
{
    memset(&cfg, 0, sizeof(cfg));
    cfg.temp_normal_c = 25.0f;
    cfg.temp_critical_c = 30.0f;
    cfg.temp_extreme_c = 35.0f;
    cfg.hysteresis_c = 1.0f;
    cfg.extreme_enabled = true;
    thermal_fsm_init(&fsm, &cfg);
}

TEST_CASE("Thermal: NORMAL com temperatura normal", "[thermal]")
{
    setUp();
    thermal_input_t in = { .sample_valid = true, .temp_c = 24.5f, .now_ms = 1000 };
    thermal_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(THERMAL_STATE_NORMAL, fsm.out.state);
    TEST_ASSERT_FALSE(fsm.out.request_heater_on);
    TEST_ASSERT_FALSE(fsm.out.request_cooler_on);
}

TEST_CASE("Thermal: ALERT quando abaixo do setpoint - histerese", "[thermal]")
{
    setUp();
    /* abaixo de normal - histerese: 25 - 1 = 24.0 */
    thermal_input_t in = { .sample_valid = true, .temp_c = 23.8f, .now_ms = 1000 };
    thermal_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(THERMAL_STATE_ALERT, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.request_heater_on);
}

TEST_CASE("Thermal: ALERT quando acima do setpoint + histerese", "[thermal]")
{
    setUp();
    thermal_input_t in = { .sample_valid = true, .temp_c = 26.2f, .now_ms = 1000 };
    thermal_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(THERMAL_STATE_ALERT, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.request_cooler_on);
}

TEST_CASE("Thermal: CRITICAL em temp critica -> force_safe_off", "[thermal]")
{
    setUp();
    thermal_input_t in = { .sample_valid = true, .temp_c = 30.5f, .now_ms = 1000 };
    thermal_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(THERMAL_STATE_CRITICAL, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.force_safe_off);
}

TEST_CASE("Thermal: EXTREME em temp extrema -> force_emergency", "[thermal]")
{
    setUp();
    thermal_input_t in = { .sample_valid = true, .temp_c = 35.1f, .now_ms = 1000 };
    thermal_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(THERMAL_STATE_EXTREME, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.force_emergency);
}

TEST_CASE("Thermal: exclusao mutua heater/cooler -> CRITICAL", "[thermal]")
{
    setUp();
    /* Simula falha: ambos ligados */
    fsm.out.request_heater_on = true;
    fsm.out.request_cooler_on = true;
    thermal_input_t in = { .sample_valid = true, .temp_c = 26.0f, .now_ms = 1000 };
    thermal_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(THERMAL_STATE_CRITICAL, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.force_safe_off);
}

TEST_CASE("Thermal: sensor fail -> SENSOR_FAIL", "[thermal]")
{
    setUp();
    thermal_input_t in = { .sample_valid = false, .temp_c = 0, .now_ms = 1000 };
    thermal_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(THERMAL_STATE_SENSOR_FAIL, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.sensor_fault);
}
