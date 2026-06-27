// @requirement RF-STORAGE-001 NVS como fonte primária de parâmetros
// @requirement RF-GLOBAL-005 Proibição de hardcode operacional
#ifndef FIRMWARE_SERVICES_CONFIG_MANAGER_H
#define FIRMWARE_SERVICES_CONFIG_MANAGER_H

#include "esp_err.h"
#include "param_catalog.h"

esp_err_t config_manager_init(void);

const thermal_params_storage_t*   config_get_thermal(void);
const ato_params_storage_t*       config_get_ato(void);
const electric_params_storage_t*  config_get_electric(void);
const plug_limits_storage_t*      config_get_plug_limits(void);
const restart_params_storage_t*   config_get_restart(void);
const feed_params_storage_t*      config_get_feed(void);
const security_params_storage_t*  config_get_security(void);
const antiflap_params_storage_t*  config_get_antiflap(void);
const selftest_params_storage_t*  config_get_selftest(void);
const system_params_storage_t*    config_get_system(void);
const calibration_params_storage_t* config_get_calibration(void);

esp_err_t config_set_thermal(const thermal_params_storage_t *p);
esp_err_t config_set_ato(const ato_params_storage_t *p);
esp_err_t config_set_electric(const electric_params_storage_t *p);
esp_err_t config_set_plug_limits(const plug_limits_storage_t *p);
esp_err_t config_set_restart(const restart_params_storage_t *p);
esp_err_t config_set_feed(const feed_params_storage_t *p);
esp_err_t config_set_security(const security_params_storage_t *p);
esp_err_t config_set_antiflap(const antiflap_params_storage_t *p);
esp_err_t config_set_selftest(const selftest_params_storage_t *p);
esp_err_t config_set_system(const system_params_storage_t *p);
esp_err_t config_set_calibration(const calibration_params_storage_t *p);

esp_err_t config_save_all(void);
esp_err_t config_load_all(void);
esp_err_t config_reset_to_defaults(void);

bool config_is_wizard_completed(void);
void config_set_wizard_completed(bool val);
void config_set_monitor_only(bool val);
uint8_t config_get_wizard_step(void);
void config_set_wizard_step(uint8_t step);

#endif
