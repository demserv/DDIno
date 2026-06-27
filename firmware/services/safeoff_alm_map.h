// @requirement RF-GLOBAL-004 Mapa causa SAFE_OFF → ALM
#ifndef FIRMWARE_SERVICES_SAFEOFF_ALM_MAP_H
#define FIRMWARE_SERVICES_SAFEOFF_ALM_MAP_H

#include <stdint.h>
#include "system_types.h"

int16_t safeoff_reason_to_alm_id(safeoff_reason_t reason);

#endif
