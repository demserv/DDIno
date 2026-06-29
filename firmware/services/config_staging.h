#ifndef FIRMWARE_SERVICES_CONFIG_STAGING_H
#define FIRMWARE_SERVICES_CONFIG_STAGING_H

#include "esp_err.h"
#include "param_catalog.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    thermal_params_storage_t    thermal;
    ato_params_storage_t        ato;
    electric_params_storage_t   electric;
    plug_limits_storage_t       plug_limits;
    restart_params_storage_t    restart;
    feed_params_storage_t       feed;
    security_params_storage_t   security;
    antiflap_params_storage_t   antiflap;
    selftest_params_storage_t   selftest;
    calibration_params_storage_t calibration;
} config_staging_t;

esp_err_t config_staging_init(void);
const config_staging_t* config_staging_get(void);
esp_err_t config_staging_set_thermal(const thermal_params_storage_t *p);
esp_err_t config_staging_set_ato(const ato_params_storage_t *p);
esp_err_t config_staging_set_electric(const electric_params_storage_t *p);
esp_err_t config_staging_set_plug_limits(const plug_limits_storage_t *p);
esp_err_t config_staging_set_restart(const restart_params_storage_t *p);
esp_err_t config_staging_set_feed(const feed_params_storage_t *p);
esp_err_t config_staging_set_security(const security_params_storage_t *p);
esp_err_t config_staging_set_antiflap(const antiflap_params_storage_t *p);
esp_err_t config_staging_set_selftest(const selftest_params_storage_t *p);
esp_err_t config_staging_set_calibration(const calibration_params_storage_t *p);
esp_err_t config_staging_commit(void);
esp_err_t config_staging_rollback(void);
bool config_staging_is_dirty(void);

#ifdef __cplusplus
}
#endif

#endif
