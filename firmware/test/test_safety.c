#include <string.h>
#include "unity.h"
#include "include/system_types.h"

/* include production source for static function access */
#include "core/safety_controller.c"

static global_state_t gs;

void setUp(void)
{
    memset(&gs, 0, sizeof(gs));
    gs.system_state = SYSTEM_STATE_NORMAL;
    gs.safeoff_reason = SAFEOFF_REASON_NONE;
}

static safety_inputs_t make_sin(void)
{
    safety_inputs_t sin;
    memset(&sin, 0, sizeof(sin));
    sin.all_sensors_valid = true;
    sin.selftest_passed = true;
    sin.manual_ack_received = true;
    sin.safeoff_cause_resolved = true;
    sin.emergency_resolved = true;
    sin.transition_cause = "test";
    return sin;
}

TEST_CASE("NORMAL mantém sem condições de falha", "[safety]")
{
    setUp();
    safety_inputs_t sin = make_sin();
    safety_controller_evaluate(&gs, &sin, 100);
    TEST_ASSERT_EQUAL(SYSTEM_STATE_NORMAL, gs.system_state);
}

TEST_CASE("safeoff_condition leva a SAFE_OFF", "[safety]")
{
    setUp();
    safety_inputs_t sin = make_sin();
    sin.safeoff_condition = true;
    sin.safeoff_reason_if_any = SAFEOFF_REASON_THERMAL_CRITICAL;
    safety_controller_evaluate(&gs, &sin, 100);
    TEST_ASSERT_EQUAL(SYSTEM_STATE_SAFE_OFF, gs.system_state);
    TEST_ASSERT_EQUAL(SAFEOFF_REASON_THERMAL_CRITICAL, gs.safeoff_reason);
}

TEST_CASE("emergency_condition leva a EMERGENCY", "[safety]")
{
    setUp();
    safety_inputs_t sin = make_sin();
    sin.emergency_condition = true;
    safety_controller_evaluate(&gs, &sin, 100);
    TEST_ASSERT_EQUAL(SYSTEM_STATE_EMERGENCY, gs.system_state);
}

TEST_CASE("SAFE_OFF mantém enquanto causa ativa", "[safety]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_SAFE_OFF;
    gs.safeoff_reason = SAFEOFF_REASON_THERMAL_CRITICAL;
    safety_inputs_t sin = make_sin();
    sin.safeoff_condition = true;
    sin.safeoff_cause_resolved = false;
    safety_controller_evaluate(&gs, &sin, 200);
    TEST_ASSERT_EQUAL(SYSTEM_STATE_SAFE_OFF, gs.system_state);
}

TEST_CASE("can_exit_safeoff requer 10s estabilizacao", "[safety]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_SAFE_OFF;
    safety_inputs_t sin = make_sin();
    sin.cause_resolved_at_ms = 5000;
    bool ok = safety_controller_can_exit_safeoff(&gs, &sin, 15);
    TEST_ASSERT_TRUE(ok);
}

TEST_CASE("can_exit_safeoff bloqueia sem ACK manual", "[safety]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_SAFE_OFF;
    safety_inputs_t sin = make_sin();
    sin.manual_ack_received = false;
    sin.cause_resolved_at_ms = 5000;
    bool ok = safety_controller_can_exit_safeoff(&gs, &sin, 15);
    TEST_ASSERT_FALSE(ok);
}

TEST_CASE("can_exit_emergency requer 30s estabilizacao + ACK", "[safety]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_EMERGENCY;
    safety_inputs_t sin = make_sin();
    sin.cause_resolved_at_ms = 5000;
    bool ok = safety_controller_can_exit_emergency(&gs, &sin, 40);
    TEST_ASSERT_TRUE(ok);
}

TEST_CASE("can_exit_emergency bloqueia sem sensores validos", "[safety]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_EMERGENCY;
    safety_inputs_t sin = make_sin();
    sin.all_sensors_valid = false;
    sin.cause_resolved_at_ms = 5000;
    bool ok = safety_controller_can_exit_emergency(&gs, &sin, 40);
    TEST_ASSERT_FALSE(ok);
}
