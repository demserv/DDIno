/* @requirement RF-AUDIT-SEC-001 RF-GLOBAL-002 */
#include "unity.h"
#include "core/safety_controller.h"

TEST_CASE("system_state_to_str cobre todos os estados", "[safety]")
{
    TEST_ASSERT_EQUAL_STRING("NORMAL",    system_state_to_str(SYSTEM_STATE_NORMAL));
    TEST_ASSERT_EQUAL_STRING("DEGRADED",  system_state_to_str(SYSTEM_STATE_DEGRADED));
    TEST_ASSERT_EQUAL_STRING("SAFE_OFF",  system_state_to_str(SYSTEM_STATE_SAFE_OFF));
    TEST_ASSERT_EQUAL_STRING("EMERGENCY", system_state_to_str(SYSTEM_STATE_EMERGENCY));
}
