#ifndef FIRMWARE_SERVICES_CONFIG_ROOT_H
#define FIRMWARE_SERVICES_CONFIG_ROOT_H

#include "esp_err.h"
#include "param_catalog.h"
#include <stdint.h>
#include <stdbool.h>

#define CONFIG_ROOT_SCHEMA_VERSION "1.0.0"
#define CONFIG_ROOT_NVS_NS         "cfg_root"
#define CONFIG_ROOT_NVS_KEY        "root"

typedef struct {
    uint32_t crc32;
    char     schema_version[16];
    thermal_params_storage_t    thermal;
    ato_params_storage_t        ato;
    electric_params_storage_t   electric;
    plug_limits_storage_t       plug_limits;
    restart_params_storage_t    restart;
    feed_params_storage_t       feed;
    security_params_storage_t   security;
    antiflap_params_storage_t   antiflap;
    selftest_params_storage_t   selftest;
    system_params_storage_t     system;
    calibration_params_storage_t calibration;
    ph_params_storage_t         ph;
} config_root_t;

esp_err_t config_root_compute_crc(config_root_t *root);

bool config_root_validate(const config_root_t *root);

esp_err_t config_root_load(config_root_t *root);

esp_err_t config_root_save(const config_root_t *root);

esp_err_t config_root_commit(config_root_t *root);

esp_err_t config_root_rollback(config_root_t *root);

#endif
