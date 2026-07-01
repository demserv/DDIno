#include <string.h>
// @requirement RF-RTM-003 TC-CMD-VAL-001 Validação de comandos
#include "unity.h"
#include "global_state.h"
#include "command_validator.h"
#include "alm_ids.h"

#include "services/command_validator.c"

static global_state_t gs;

void setUp(void)
{
    memset(&gs, 0, sizeof(gs));
    gs.wizard_completed = true;
    gs.system_state = SYSTEM_STATE_NORMAL;
    gs.selftest_passed = true;
}

TEST_CASE("ack_all bloqueado em SAFE_OFF", "[cmd_validator]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_SAFE_OFF;
    cmd_validation_t cv = command_validator_can_ack_alert(&gs, 0);
    TEST_ASSERT_FALSE(cv.allowed);
    TEST_ASSERT_NOT_NULL(cv.error_code);
    TEST_ASSERT_EQUAL_STRING("SAFE_MODE_ACTIVE", cv.error_code);
}

TEST_CASE("ack_all bloqueado em EMERGENCY", "[cmd_validator]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_EMERGENCY;
    cmd_validation_t cv = command_validator_can_ack_alert(&gs, 0);
    TEST_ASSERT_FALSE(cv.allowed);
}

TEST_CASE("ack individual permitido em SAFE_OFF", "[cmd_validator]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_SAFE_OFF;
    cmd_validation_t cv = command_validator_can_ack_alert(&gs, ALM_026);
    TEST_ASSERT_TRUE(cv.allowed);
}

TEST_CASE("toggle plug bloqueado em SAFE_OFF", "[cmd_validator]")
{
    setUp();
    gs.system_state = SYSTEM_STATE_SAFE_OFF;
    cmd_validation_t cv = command_validator_can_toggle_plug(&gs, 3, true);
    TEST_ASSERT_FALSE(cv.allowed);
    TEST_ASSERT_EQUAL_STRING("SAFE_MODE_ACTIVE", cv.error_code);
}

TEST_CASE("P01 exige dupla confirmacao", "[cmd_validator]")
{
    setUp();
    cmd_validation_t cv = command_validator_can_toggle_plug(&gs, 1, true);
    TEST_ASSERT_TRUE(cv.allowed);
    TEST_ASSERT_TRUE(cv.requires_double_confirmation);
}

TEST_CASE("feed bloqueado em monitor_only", "[cmd_validator]")
{
    setUp();
    gs.monitor_only_mode = true;
    cmd_validation_t cv = command_validator_can_start_feed(&gs);
    TEST_ASSERT_FALSE(cv.allowed);
}
