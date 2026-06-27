// @requirement RF-UTIL-001 a RF-UTIL-027 Utilitários de sistema
#include "util_ctl.h"
#include <string.h>

float util_map(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float util_constrain(float val, float min, float max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

uint32_t util_min_u32(uint32_t a, uint32_t b) { return (a < b) ? a : b; }
uint32_t util_max_u32(uint32_t a, uint32_t b) { return (a > b) ? a : b; }

int32_t util_clamp_i32(int32_t val, int32_t min, int32_t max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

bool util_antiflap(bool *last_stable, bool current, uint32_t *count, uint32_t threshold)
{
    if (!last_stable || !count) return false;
    if (current != *last_stable) {
        (*count)++;
        if (*count >= threshold) {
            *last_stable = current;
            *count = 0;
            return true;
        }
    } else {
        *count = 0;
    }
    return false;
}

uint16_t util_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

uint32_t util_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

void util_bcd_to_str(uint8_t bcd, char *out, size_t max_len)
{
    if (!out || max_len < 3) return;
    out[0] = '0' + ((bcd >> 4) & 0x0F);
    out[1] = '0' + (bcd & 0x0F);
    out[2] = '\0';
}

float util_celsius_to_fahrenheit(float c) { return c * 1.8f + 32.0f; }
float util_fahrenheit_to_celsius(float f) { return (f - 32.0f) / 1.8f; }

bool util_str_to_bool(const char *str)
{
    if (!str) return false;
    return (strcmp(str, "1") == 0 || strcmp(str, "true") == 0 ||
            strcmp(str, "yes") == 0 || strcmp(str, "on") == 0);
}

const char *util_bool_to_str(bool val) { return val ? "true" : "false"; }
