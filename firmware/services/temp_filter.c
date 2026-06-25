#include "services/temp_filter.h"

static float s_buffer[TEMP_FILTER_MAX_WINDOW];
static uint8_t s_head = 0;
static uint8_t s_count = 0;
static uint8_t s_window = 5;
static bool s_valid = false;

void temp_filter_init(uint8_t window_size)
{
    if (window_size < 1) window_size = 1;
    if (window_size > TEMP_FILTER_MAX_WINDOW) window_size = TEMP_FILTER_MAX_WINDOW;
    s_window = window_size;
    s_head = 0;
    s_count = 0;
    s_valid = false;
}

float temp_filter_update(float sample_c)
{
    s_buffer[s_head] = sample_c;
    s_head = (s_head + 1) % s_window;
    if (s_count < s_window) s_count++;
    if (s_count >= 2) s_valid = true;

    double sum = 0.0;
    for (uint8_t i = 0; i < s_count; i++) {
        sum += (double)s_buffer[i];
    }
    return (float)(sum / (double)s_count);
}

float temp_filter_get_current(void)
{
    if (!s_valid || s_count == 0) return 0.0f;
    double sum = 0.0;
    for (uint8_t i = 0; i < s_count; i++) {
        sum += (double)s_buffer[i];
    }
    return (float)(sum / (double)s_count);
}

bool temp_filter_is_valid(void)
{
    return s_valid;
}

void temp_filter_reset(void)
{
    s_head = 0;
    s_count = 0;
    s_valid = false;
}
