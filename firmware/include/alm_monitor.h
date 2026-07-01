#ifndef FIRMWARE_INCLUDE_ALM_MONITOR_H
#define FIRMWARE_INCLUDE_ALM_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t alm_monitor_init(void);
void alm_monitor_tick(uint64_t now_s);

#ifdef __cplusplus
}
#endif

#endif
