#ifndef FIRMWARE_SERVICES_WDT_STATS_H
#define FIRMWARE_SERVICES_WDT_STATS_H

#include <stdint.h>
#include "esp_err.h"

esp_err_t wdt_stats_init(uint64_t now_s);
uint32_t  wdt_stats_get_resets_24h(void);

#endif
