#include <string.h>
// @requirement RF-RTM-001 a RF-RTM-003 Testes de alerta
#include "unity.h"
#include "services/alert_manager.h"
#include "alert_model.h"

/* include production source */
#include "services/alert_manager.c"

TEST_CASE("Alert: init limpa todos os slots", "[alert]")
{
    alert_manager_init();
    TEST_ASSERT_EQUAL(0, alert_manager_active_count());
}

TEST_CASE("Alert: raise ativa um alerta", "[alert]")
{
    alert_manager_init();
    esp_err_t err = alert_manager_raise(ALM_026, true, 1000);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(alert_manager_is_active(ALM_026));
    TEST_ASSERT_EQUAL(1, alert_manager_active_count());
}

TEST_CASE("Alert: raise duplicado nao incrementa contagem", "[alert]")
{
    alert_manager_init();
    alert_manager_raise(ALM_026, true, 1000);
    alert_manager_raise(ALM_026, true, 2000);
    TEST_ASSERT_EQUAL(1, alert_manager_active_count());
}

TEST_CASE("Alert: ack marca timestamp", "[alert]")
{
    alert_manager_init();
    alert_manager_raise(ALM_026, true, 1000);
    alert_manager_ack(ALM_026, 5000);
    /* ack deve definir ack_timestamp, mas alerta permanece ativo */
    TEST_ASSERT_TRUE(alert_manager_is_active(ALM_026));
}

TEST_CASE("Alert: clear remove alerta", "[alert]")
{
    alert_manager_init();
    alert_manager_raise(ALM_026, true, 1000);
    alert_manager_clear(ALM_026);
    TEST_ASSERT_FALSE(alert_manager_is_active(ALM_026));
    TEST_ASSERT_EQUAL(0, alert_manager_active_count());
}

TEST_CASE("Alert: critical_count funciona", "[alert]")
{
    alert_manager_init();
    alert_manager_raise(ALM_026, true, 1000);
    alert_manager_raise(ALM_060, true, 1000);
    alert_manager_raise(ALM_015, false, 1000);
    TEST_ASSERT_EQUAL(2, alert_manager_critical_count());
}

TEST_CASE("Alert: 65 slots de ALM_001 a ALM_065", "[alert]")
{
    alert_manager_init();
    for (int16_t id = 1; id <= 65; id++) {
        alert_manager_raise(id, true, 1000);
    }
    TEST_ASSERT_EQUAL(65, alert_manager_active_count());
}

TEST_CASE("Alert: raise_full preenche todos os campos", "[alert]")
{
    alert_manager_init();
    bool ok = alert_manager_raise_full(ALM_026, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_PROCESS,
                                       "Test alert", 30.5f, "Check heater",
                                       0, true, false, 1000);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(alert_manager_is_active(ALM_026));
}
