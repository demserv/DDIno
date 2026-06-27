// @requirement RNF-SECURITY-001 Rate limiting por IP
#ifndef FIRMWARE_WEB_API_RATE_LIMIT_H
#define FIRMWARE_WEB_API_RATE_LIMIT_H

#include <stdbool.h>
#include <stdint.h>

#define RATE_LIMIT_WINDOW_S    60
#define RATE_LIMIT_MAX_REQ     30
#define RATE_LIMIT_TRACKED_IPS 16

bool rate_limit_check(uint32_t ip_addr);
void rate_limit_reset(uint32_t ip_addr);

#endif
