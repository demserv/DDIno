#include <string.h>
// @requirement RF-RTM-003 TC-ALM-MON-001 ALM-006 SD e ALM-008 heap
#include "unity.h"
#include "global_state.h"
#include "alm_monitor.h"
#include "alert_manager.h"
#include "health_matrix.h"
#include "circuit_breaker.h"
#include "alm_ids.h"
#include "param_catalog.h"
#include "driver_pzem.h"
#include "plug_model.h"

static global_state_t s_gs;
pzem_data_t g_pzem;

const global_state_t *global_state_get_snapshot_ptr(void)
{
    return &s_gs;
}

const electric_params_storage_t *config_get_electric(void)
{
    static electric_params_storage_t ep = {
        .total_power_limit_w = 1200.0f,
        .per_plug_current_limit_a = 10.0f,
        .total_current_limit_a = 15.0f,
        .pf_min = 0.5f,
        .overvoltage_limit_v = 250.0f,
        .undervoltage_limit_v = 190.0f,
    };
    return &ep;
}

esp_err_t storage_sd_get_space(uint64_t *total, uint64_t *free)
{
    if (total) *total = 1000;
    if (free) *free = 500;
    return ESP_OK;
}

plug_model_t *plug_manager_get(plug_id_t id)
{
    (void)id;
    return NULL;
}

esp_err_t ph_sensor_read(float *ph, bool *valid)
{
    if (valid) *valid = false;
    (void)ph;
    return ESP_ERR_INVALID_STATE;
}

bool ph_sensor_is_ok(void)
{
    return false;
}

float cdn_energy_get_total_wh(void)
{
    return 0.0f;
}

bool relay_mcp23017_ok(void)
{
    return true;
}

cb_state_t circuit_breaker_get_state(cb_bus_id_t id)
{
    (void)id;
    return CB_STATE_CLOSED;
}

#include "services/alm_monitor.c"

void setUp(void)
{
    alert_manager_init();
    health_matrix_init();
    alm_monitor_init();
    memset(&s_gs, 0, sizeof(s_gs));
    memset(&g_pzem, 0, sizeof(g_pzem));
    strncpy(s_gs.config_schema_version, "1.0.0", sizeof(s_gs.config_schema_version));
    s_gs.ui_ok = true;
}

TEST_CASE("ALM-006 raise quando SD indisponivel", "[alm_monitor]")
{
    setUp();
    s_gs.sd_ok = false;
    alm_monitor_tick(10);
    TEST_ASSERT_TRUE(alert_manager_is_active(ALM_006));
}

TEST_CASE("ALM-006 clear quando SD ok (sem flap)", "[alm_monitor]")
{
    setUp();
    s_gs.sd_ok = false;
    alm_monitor_tick(10);
    TEST_ASSERT_TRUE(alert_manager_is_active(ALM_006));

    s_gs.sd_ok = true;
    alm_monitor_tick(11);
    TEST_ASSERT_FALSE(alert_manager_is_active(ALM_006));
}

TEST_CASE("ALM-006 nao re-raise a cada tick com SD ok", "[alm_monitor]")
{
    setUp();
    s_gs.sd_ok = true;
    for (uint64_t t = 10; t < 20; t++) {
        alm_monitor_tick(t);
    }
    TEST_ASSERT_FALSE(alert_manager_is_active(ALM_006));
}

TEST_CASE("ALM-008 nao ativo com heap saudavel", "[alm_monitor]")
{
    setUp();
    s_gs.sd_ok = true;
    alm_monitor_tick(35);
    alm_monitor_tick(65);
    TEST_ASSERT_FALSE(alert_manager_is_active(ALM_008));
}
