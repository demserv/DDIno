#ifndef FIRMWARE_INCLUDE_EVENT_LOG_H
#define FIRMWARE_INCLUDE_EVENT_LOG_H

#include <stdint.h>

#define EVENT_LOG_SOURCE_MAX_LEN   (32U)
#define EVENT_LOG_MESSAGE_MAX_LEN  (160U)

typedef enum {
    EVENT_SEVERITY_INFO = 0,
    EVENT_SEVERITY_WARNING,
    EVENT_SEVERITY_ERROR,
    EVENT_SEVERITY_CRITICAL
} event_severity_t;

typedef struct {
    uint64_t         timestamp;
    event_severity_t severity;
    char             source[EVENT_LOG_SOURCE_MAX_LEN];
    char             msg[EVENT_LOG_MESSAGE_MAX_LEN];
} event_log_entry_t;

#endif
