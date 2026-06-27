// @requirement RF-GLOBAL-005 Schema de configuração sem hardcode operacional
#ifndef FIRMWARE_INCLUDE_CONFIG_SCHEMA_H
#define FIRMWARE_INCLUDE_CONFIG_SCHEMA_H

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_SCHEMA_VERSION_MAX_LEN   (16U)
#define CONFIG_WIZARD_PROFILE_NAME_LEN  (32U)

typedef enum {
    MAINS_VOLTAGE_127V = 127,
    MAINS_VOLTAGE_220V = 220
} mains_voltage_t;

typedef struct {
    char            schema_version[CONFIG_SCHEMA_VERSION_MAX_LEN];
    bool            wizard_completed;
    mains_voltage_t mains_voltage;
    bool            monitor_only_mode;
    bool            maintenance_mode;
} config_schema_t;

typedef enum {
    SCHEMA_CHECK_OK = 0,
    SCHEMA_CHECK_INVALID,
    SCHEMA_CHECK_INCOMPATIBLE
} schema_check_result_t;

#endif
