#include <string.h>
// @requirement RF-RTM-001 a RF-RTM-003 Testes de Plug Manager
#include "unity.h"
#include "services/plug_manager.h"
#include "driver_relay.h"

#include "drivers/driver_relay.c"
#include "services/plug_manager.c"

/* Stub: MCP23017 implementations not linked separately in test build */
esp_err_t mcp23017_init(i2c_port_t port, uint8_t addr) { (void)port; (void)addr; return ESP_OK; }
esp_err_t mcp23017_set_relay_p03_p10_mask(uint8_t mask) { (void)mask; return ESP_OK; }
void mcp23017_get_input_mask(uint16_t *mask) { if (mask) *mask = 0; }

void setUp(void)
{
    plug_manager_init();
    relay_init_safe();
}

TEST_CASE("PlugMgr: init cria 10 plugs em AUTO", "[plug]")
{
    setUp();
    for (int i = 1; i <= 10; i++) {
        plug_mode_t mode = plug_manager_get_mode((plug_id_t)i);
        TEST_ASSERT_EQUAL(PLUG_MODE_AUTO, mode);
    }
}

TEST_CASE("PlugMgr: set_mode altera modo", "[plug]")
{
    setUp();
    esp_err_t err = plug_manager_set_mode(PLUG_ID_P01, PLUG_MODE_MANUAL);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(PLUG_MODE_MANUAL, plug_manager_get_mode(PLUG_ID_P01));
}

TEST_CASE("PlugMgr: set_mode rejeita id invalido", "[plug]")
{
    setUp();
    esp_err_t err = plug_manager_set_mode((plug_id_t)99, PLUG_MODE_MANUAL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

TEST_CASE("PlugMgr: toggle liga/desliga tomada", "[plug]")
{
    setUp();
    esp_err_t err = plug_manager_toggle(PLUG_ID_P01, true);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(plug_manager_get_effective_state(PLUG_ID_P01));
}

TEST_CASE("PlugMgr: toggle bloqueado em safe_off", "[plug]")
{
    setUp();
    plug_manager_tick(100, SYSTEM_STATE_SAFE_OFF, false);
    esp_err_t err = plug_manager_toggle(PLUG_ID_P01, true);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

TEST_CASE("PlugMgr: active_count apos toggle", "[plug]")
{
    setUp();
    TEST_ASSERT_EQUAL(0, plug_manager_active_count());
    plug_manager_toggle(PLUG_ID_P01, true);
    plug_manager_toggle(PLUG_ID_P02, true);
    TEST_ASSERT_EQUAL(2, plug_manager_active_count());
}

TEST_CASE("PlugMgr: safe_off desliga todas", "[plug]")
{
    setUp();
    plug_manager_toggle(PLUG_ID_P01, true);
    plug_manager_toggle(PLUG_ID_P02, true);
    plug_manager_apply_safe_off();
    TEST_ASSERT_FALSE(plug_manager_get_effective_state(PLUG_ID_P01));
    TEST_ASSERT_FALSE(plug_manager_get_effective_state(PLUG_ID_P02));
}

TEST_CASE("PlugMgr: get retorna NULL para id invalido", "[plug]")
{
    setUp();
    plug_model_t *p = plug_manager_get((plug_id_t)99);
    TEST_ASSERT_NULL(p);
}

TEST_CASE("PlugMgr: get retorna ponteiro valido", "[plug]")
{
    setUp();
    plug_model_t *p = plug_manager_get(PLUG_ID_P01);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL(PLUG_ID_P01, p->id);
}

TEST_CASE("PlugMgr: set_thermal_request afeta modo AUTO", "[plug]")
{
    setUp();
    plug_manager_set_thermal_request(PLUG_ID_P02, true, false);
    /* O P02 (aquecedor) deve ligar quando heater_on=true no tick */
    plug_manager_tick(100, SYSTEM_STATE_NORMAL, false);
    TEST_ASSERT_TRUE(plug_manager_get_effective_state(PLUG_ID_P02));
}
