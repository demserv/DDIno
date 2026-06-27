#include <string.h>
// @requirement RF-RTM-001 a RF-RTM-003 Testes de Feed
#include "unity.h"
#include "fsm/feed_fsm.h"

/* include production source */
#include "fsm/feed_fsm.c"

static feed_fsm_t fsm;

void setUp(void)
{
    feed_fsm_init(&fsm, 600, 120);
}

TEST_CASE("Feed: estado inicial IDLE", "[feed]")
{
    setUp();
    TEST_ASSERT_EQUAL(FEED_STATE_IDLE, fsm.state);
}

TEST_CASE("Feed: start transiciona para ACTIVE", "[feed]")
{
    setUp();
    bool ok = feed_fsm_start(&fsm, 1000);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(FEED_STATE_ACTIVE, fsm.state);
}

TEST_CASE("Feed: stop transiciona para COOLDOWN", "[feed]")
{
    setUp();
    feed_fsm_start(&fsm, 1000);
    feed_fsm_stop(&fsm);
    TEST_ASSERT_EQUAL(FEED_STATE_COOLDOWN, fsm.state);
}

TEST_CASE("Feed: can_start retorna false se ja ativo", "[feed]")
{
    setUp();
    feed_fsm_start(&fsm, 1000);
    bool ok = feed_fsm_can_start(&fsm, 2000);
    TEST_ASSERT_FALSE(ok);
}

TEST_CASE("Feed: update apos duration vai para COOLDOWN", "[feed]")
{
    setUp();
    feed_fsm_start(&fsm, 1000);
    /* duration = 600s, entao depois de 600001ms deve ir para cooldown */
    feed_fsm_update(&fsm, 602000);
    TEST_ASSERT_EQUAL(FEED_STATE_COOLDOWN, fsm.state);
}

TEST_CASE("Feed: cooldown expira volta para IDLE", "[feed]")
{
    setUp();
    feed_fsm_start(&fsm, 1000);
    feed_fsm_update(&fsm, 602000);
    TEST_ASSERT_EQUAL(FEED_STATE_COOLDOWN, fsm.state);
    /* cooldown = 120s, apos 120001ms */
    feed_fsm_update(&fsm, 725000);
    TEST_ASSERT_EQUAL(FEED_STATE_IDLE, fsm.state);
}

TEST_CASE("Feed: anti-flap max 2 feeds por hora", "[feed]")
{
    setUp();
    feed_fsm_start(&fsm, 1000);
    TEST_ASSERT_EQUAL(FEED_STATE_ACTIVE, fsm.state);

    /* Finaliza primeiro feed */
    feed_fsm_update(&fsm, 602000);
    feed_fsm_update(&fsm, 725000); /* cooldown expira */
    TEST_ASSERT_EQUAL(FEED_STATE_IDLE, fsm.state);

    /* Segundo feed */
    bool ok = feed_fsm_start(&fsm, 730000);
    TEST_ASSERT_TRUE(ok);
    feed_fsm_update(&fsm, 1331000);
    feed_fsm_update(&fsm, 1455000); /* cooldown expira */
    TEST_ASSERT_EQUAL(FEED_STATE_IDLE, fsm.state);

    /* Terceiro feed deve falhar (max 2/h) */
    ok = feed_fsm_can_start(&fsm, 1460000);
    TEST_ASSERT_FALSE(ok);
}

TEST_CASE("Feed: remaining_s retorna duracao quando ativo", "[feed]")
{
    setUp();
    feed_fsm_start(&fsm, 1000);
    uint32_t rem = feed_fsm_remaining_s(&fsm, 1000);
    TEST_ASSERT_EQUAL(600, rem);
}
