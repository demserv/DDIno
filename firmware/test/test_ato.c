#include <string.h>
#include "unity.h"
#include "fsm/ato_fsm.h"

/* include production source */
#include "fsm/ato_fsm.c"

static ato_fsm_t fsm;
static ato_params_t cfg;

void setUp(void)
{
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.low_level_adc = 500;
    cfg.high_level_adc = 3000;
    cfg.overflow_margin_adc = 200;
    cfg.refill_timeout_s = 60;
    ato_fsm_init(&fsm, &cfg);
}

TEST_CASE("ATO: NORMAL quando nivel OK", "[ato]")
{
    setUp();
    ato_input_t in = { .sample_valid = true, .level_adc = 2000, .now_ms = 1000 };
    ato_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(ATO_STATE_NORMAL, fsm.out.state);
}

TEST_CASE("ATO: DISABLED quando config desabilitado", "[ato]")
{
    setUp();
    cfg.enabled = false;
    ato_fsm_init(&fsm, &cfg);
    ato_input_t in = { .sample_valid = true, .level_adc = 2000, .now_ms = 1000 };
    ato_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(ATO_STATE_DISABLED, fsm.out.state);
}

TEST_CASE("ATO: REFILLING quando nivel baixo", "[ato]")
{
    setUp();
    ato_input_t in = { .sample_valid = true, .level_adc = 200, .now_ms = 1000 };
    ato_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(ATO_STATE_REFILLING, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.pump_request_on);
}

TEST_CASE("ATO: OVERFLOW quando nivel alto + margem", "[ato]")
{
    setUp();
    ato_input_t in = { .sample_valid = true, .level_adc = 3500, .now_ms = 1000 };
    ato_fsm_update(&fsm, &in);
    /* high=3000 + overflow_margin=200 = 3200 -> 3500 > 3200 */
    TEST_ASSERT_EQUAL(ATO_STATE_OVERFLOW, fsm.out.state);
    TEST_ASSERT_TRUE(fsm.out.force_safe_off);
}

TEST_CASE("ATO: BLOCKED apos refill timeout", "[ato]")
{
    setUp();
    ato_input_t in = { .sample_valid = true, .level_adc = 200, .now_ms = 1000 };
    ato_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(ATO_STATE_REFILLING, fsm.out.state);

    /* timeout em 60s */
    in.now_ms = 70000;
    ato_fsm_update(&fsm, &in);
    TEST_ASSERT_EQUAL(ATO_STATE_BLOCKED, fsm.out.state);
}
