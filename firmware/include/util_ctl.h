// @requirement RF-UTIL-001 a RF-UTIL-027 Utilitários de sistema: modelos, validação, conversão, CRC
#ifndef FIRMWARE_INCLUDE_UTIL_CTL_H
#define FIRMWARE_INCLUDE_UTIL_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

float util_map(float x, float in_min, float in_max, float out_min, float out_max);
float util_constrain(float val, float min, float max);
uint32_t util_min_u32(uint32_t a, uint32_t b);
uint32_t util_max_u32(uint32_t a, uint32_t b);
int32_t util_clamp_i32(int32_t val, int32_t min, int32_t max);
bool util_antiflap(bool *last_stable, bool current, uint32_t *count, uint32_t threshold);
uint16_t util_crc16(const uint8_t *data, uint16_t len);
uint32_t util_crc32(const uint8_t *data, uint32_t len);
void util_bcd_to_str(uint8_t bcd, char *out, size_t max_len);
float util_celsius_to_fahrenheit(float c);
float util_fahrenheit_to_celsius(float f);
bool util_str_to_bool(const char *str);
const char *util_bool_to_str(bool val);

#ifdef __cplusplus
}
#endif

#endif
