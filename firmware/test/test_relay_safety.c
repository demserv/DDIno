/* @requirement RF-PLUG-001 / RF-GLOBAL-002 / RNF-SECURITY-001
 * TC-RELAY-SAFETY-001: a rota única de relés nega acionamento em estados
 * de segurança e quando o self-test não passou. */
#include <string.h>
#include "unity.h"
#include "esp_err.h"
#include "global_state.h"
#include "relay_abstraction.h"

static global_state_t s_gs_test;

static void bind_state(system_state_t st, bool selftest)
{
    memset(&s_gs_test, 0, sizeof(s_gs_test));
    s_gs_test.system_state = st;
    s_gs_test.selftest_passed = selftest;
    global_state_bind(&s_gs_test);
    relay_abstraction_init();
}

TEST_CASE("TC-RELAY-SAFETY-001 SAFE_OFF bloqueia ON", "[relay][safety]")
{
    bind_state(SYSTEM_STATE_SAFE_OFF, true);
    esp_err_t err = relay_abstraction_set(RELAY_ID_P03, true);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    TEST_ASSERT_FALSE(relay_abstraction_is_on(RELAY_ID_P03));
}

TEST_CASE("TC-RELAY-SAFETY-001 EMERGENCY bloqueia ON", "[relay][safety]")
{
    bind_state(SYSTEM_STATE_EMERGENCY, true);
    esp_err_t err = relay_abstraction_set(RELAY_ID_P03, true);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    TEST_ASSERT_FALSE(relay_abstraction_is_on(RELAY_ID_P03));
}

TEST_CASE("TC-RELAY-SAFETY-001 self-test reprovado bloqueia ON", "[relay][safety]")
{
    bind_state(SYSTEM_STATE_NORMAL, false);
    esp_err_t err = relay_abstraction_set(RELAY_ID_P03, true);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    TEST_ASSERT_FALSE(relay_abstraction_is_on(RELAY_ID_P03));
}

TEST_CASE("TC-RELAY-SAFETY-001 rele critico sem confirmacao dupla bloqueia ON", "[relay][safety]")
{
    bind_state(SYSTEM_STATE_NORMAL, true);
    /* P01/P02 são críticos: sem arm prévio, ON é negado. */
    esp_err_t err = relay_abstraction_set(RELAY_ID_P01, true);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    TEST_ASSERT_FALSE(relay_abstraction_is_on(RELAY_ID_P01));
}
