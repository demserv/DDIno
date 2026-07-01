#include <string.h>
// @requirement RF-RTM-003 TC-SAFEOFF-ACK-001 Duplo ACK crítico para sair de SAFE_OFF
#include "unity.h"
#include "global_state.h"
#include "safe_state_ack.h"
#include "services/alert_manager.h"

global_state_t g_gs;

void setUp(void)
{
    alert_manager_init();
    memset(&g_gs, 0, sizeof(g_gs));
    g_gs.system_state = SYSTEM_STATE_SAFE_OFF;
    strncpy(g_gs.safeoff_source_alm, "ALM-026", sizeof(g_gs.safeoff_source_alm));
    safe_state_ack_on_enter_safeoff(&g_gs);
    alert_manager_raise_full(ALM_026, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_PROCESS,
                             "Temp critica", 35.0f, "Intervencao", 0, true, false, 100);
}

TEST_CASE("SAFE_OFF sem ACK manual retorna false", "[safe_state_ack]")
{
    setUp();
    TEST_ASSERT_FALSE(safe_state_ack_manual_received(&g_gs));
}

TEST_CASE("primeiro ACK critico nao libera SAFE_OFF", "[safe_state_ack]")
{
    setUp();
    alert_manager_ext_ack_critical(ALM_026, 200);
    safe_state_ack_on_alert_ack(ALM_026, 200);
    TEST_ASSERT_FALSE(safe_state_ack_manual_received(&g_gs));
    TEST_ASSERT_EQUAL(ACK_STAGE_FIRST, alert_manager_ext_get_ack_stage(ALM_026));
}

TEST_CASE("segundo ACK critico libera SAFE_OFF", "[safe_state_ack]")
{
    setUp();
    alert_manager_ext_ack_critical(ALM_026, 200);
    safe_state_ack_on_alert_ack(ALM_026, 200);
    alert_manager_ext_ack_critical(ALM_026, 201);
    safe_state_ack_on_alert_ack(ALM_026, 201);
    TEST_ASSERT_TRUE(safe_state_ack_manual_received(&g_gs));
    TEST_ASSERT_EQUAL(ACK_STAGE_CONFIRMED, alert_manager_ext_get_ack_stage(ALM_026));
}

TEST_CASE("EMERGENCY exige ACK manual apos enter", "[safe_state_ack]")
{
    setUp();
    g_gs.system_state = SYSTEM_STATE_EMERGENCY;
    safe_state_ack_on_enter_emergency();
    TEST_ASSERT_FALSE(safe_state_ack_manual_received(&g_gs));
}

TEST_CASE("NORMAL sempre retorna ACK ok", "[safe_state_ack]")
{
    setUp();
    g_gs.system_state = SYSTEM_STATE_NORMAL;
    TEST_ASSERT_TRUE(safe_state_ack_manual_received(&g_gs));
}
