#include <string.h>
#include "unity.h"
#include "services/electric_fsm.h"
#include "alm_ids.h"

/* include production source */
#include "services/electric_fsm.c"

static electric_fsm_t fsm;
static electric_params_t cfg;

void setUp(void)
{
    memset(&cfg, 0, sizeof(cfg));
    cfg.total_power_limit_w = 1500;
    cfg.per_plug_current_limit_a = 10.0f;
    cfg.hysteresis_w = 50;
    cfg.overvoltage_limit_v = 135.0f;
    cfg.undervoltage_limit_v = 105.0f;
    cfg.overvoltage_time_s = 3;
    cfg.undervoltage_time_s = 5;
    cfg.total_current_limit_a = 12.0f;
    cfg.total_current_time_s = 5;
    cfg.fator_curto = 3.0f;
    cfg.tempo_deteccao_curto_ms = 500;
    electric_fsm_init(&fsm, &cfg);
}

TEST_CASE("Electric: NORMAL sem condicoes de falha", "[electric]")
{
    setUp();
    electric_input_t in;
    memset(&in, 0, sizeof(in));
    in.sample_valid = true;
    in.total_power_w = 500;
    in.voltage_v = 127.0f;
    in.now_ms = 1000;
    electric_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(ELECTRIC_STATE_NORMAL, fsm.out.state);
}

TEST_CASE("Electric: curto-circuito detectado apos 3 amostras consecutivas", "[electric]")
{
    setUp();
    electric_input_t in;
    memset(&in, 0, sizeof(in));
    in.sample_valid = true;
    in.total_power_w = 500;
    in.voltage_v = 127.0f;
    in.plug_count = 1;
    in.plug_currents_a[0] = 35.0f;
    in.now_ms = 1000;

    electric_fsm_update(&fsm, &in); in.now_ms = 1100;
    electric_fsm_update(&fsm, &in); in.now_ms = 1200;
    electric_fsm_update(&fsm, &in); in.now_ms = 1300;

    TEST_ASSERT_EQUAL(ELECTRIC_STATE_SHORT_CIRCUIT, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.force_safe_off);
}

TEST_CASE("Electric: overvoltage detectado apos tempo configurado", "[electric]")
{
    setUp();
    electric_input_t in;
    memset(&in, 0, sizeof(in));
    in.sample_valid = true;
    in.total_power_w = 500;
    in.voltage_v = 140.0f; /* > 135V */
    in.now_ms = 1000;

    /* overvoltage_time_s = 3, precisa de 3000ms */
    electric_fsm_update(&fsm, &in); in.now_ms = 2000;
    electric_fsm_update(&fsm, &in); in.now_ms = 3000;
    TEST_ASSERT_EQUAL(ELECTRIC_STATE_NORMAL, fsm.out.state);

    electric_fsm_update(&fsm, &in); in.now_ms = 4500;
    TEST_ASSERT_EQUAL(ELECTRIC_STATE_OVERVOLTAGE, fsm.out.state);
}

TEST_CASE("Electric: undervoltage detectado apos tempo configurado", "[electric]")
{
    setUp();
    electric_input_t in;
    memset(&in, 0, sizeof(in));
    in.sample_valid = true;
    in.total_power_w = 500;
    in.voltage_v = 100.0f;
    in.now_ms = 1000;

    for (int i = 0; i < 6; i++) {
        electric_fsm_update(&fsm, &in);
        in.now_ms += 1000;
    }
    TEST_ASSERT_EQUAL(ELECTRIC_STATE_UNDERVOLTAGE, fsm.out.state);
}
