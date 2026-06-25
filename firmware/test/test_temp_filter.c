#include <string.h>
#include "unity.h"
#include "services/temp_filter.h"

/* include production source */
#include "services/temp_filter.c"

TEST_CASE("TempFilter: init com janela 5", "[temp_filter]")
{
    temp_filter_init(5);
    TEST_ASSERT_FALSE(temp_filter_is_valid());
}

TEST_CASE("TempFilter: media movel de 3 amostras", "[temp_filter]")
{
    temp_filter_init(3);
    float r1 = temp_filter_update(10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, r1);

    float r2 = temp_filter_update(20.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, r2);

    float r3 = temp_filter_update(30.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, r3);
}

TEST_CASE("TempFilter: media movel de 5 amostras", "[temp_filter]")
{
    temp_filter_init(5);
    temp_filter_update(10.0f);
    temp_filter_update(20.0f);
    temp_filter_update(30.0f);
    temp_filter_update(40.0f);
    float r = temp_filter_update(50.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, r);
}

TEST_CASE("TempFilter: get_current apos updates", "[temp_filter]")
{
    temp_filter_init(3);
    temp_filter_update(25.0f);
    temp_filter_update(26.0f);
    temp_filter_update(27.0f);
    float cur = temp_filter_get_current();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 26.0f, cur);
    TEST_ASSERT_TRUE(temp_filter_is_valid());
}

TEST_CASE("TempFilter: reset zera estado", "[temp_filter]")
{
    temp_filter_init(3);
    temp_filter_update(25.0f);
    temp_filter_update(26.0f);
    temp_filter_reset();
    TEST_ASSERT_FALSE(temp_filter_is_valid());
}

TEST_CASE("TempFilter: janela 1 retorna o proprio valor", "[temp_filter]")
{
    temp_filter_init(1);
    float r = temp_filter_update(42.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 42.5f, r);
}

TEST_CASE("TempFilter: janela maxima 20 funciona", "[temp_filter]")
{
    temp_filter_init(20);
    for (int i = 0; i < 20; i++) {
        temp_filter_update((float)(i * 2));
    }
    float r = temp_filter_get_current();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 19.0f, r); /* (0+2+...+38)/20 = 380/20 = 19 */
}
