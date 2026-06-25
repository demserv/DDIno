#ifndef FIRMWARE_INCLUDE_TIME_MANAGER_H
#define FIRMWARE_INCLUDE_TIME_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "system_types.h"

typedef struct {
    bool          time_valid;
    time_source_t time_source;
    uint64_t      unix_time_s;
} time_snapshot_t;

#endif
