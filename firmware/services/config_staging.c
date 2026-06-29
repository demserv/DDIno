#include "config_staging.h"

#include <string.h>

#include "config_manager.h"

static config_staging_t s_staging;
static config_staging_t s_backup;
static bool s_initialized = false;
static bool s_dirty = false;

esp_err_t config_staging_init(void)
{
    memset(&s_staging, 0, sizeof(s_staging));
    memset(&s_backup, 0, sizeof(s_backup));
    s_initialized = true;
    s_dirty = false;
    return ESP_OK;
}

const config_staging_t* config_staging_get(void)
{
    if (!s_initialized) return NULL;
    return &s_staging;
}

static esp_err_t copy_field(void *dst, const void *src, size_t sz)
{
    if (!s_initialized || !dst || !src) return ESP_ERR_INVALID_STATE;
    if (!s_dirty) {
        memcpy(&s_backup, &s_staging, sizeof(s_staging));
        s_dirty = true;
    }
    memcpy(dst, src, sz);
    return ESP_OK;
}

esp_err_t config_staging_set_thermal(const thermal_params_storage_t *p)
{
    return copy_field(&s_staging.thermal, p, sizeof(thermal_params_storage_t));
}

esp_err_t config_staging_set_ato(const ato_params_storage_t *p)
{
    return copy_field(&s_staging.ato, p, sizeof(ato_params_storage_t));
}

esp_err_t config_staging_set_electric(const electric_params_storage_t *p)
{
    return copy_field(&s_staging.electric, p, sizeof(electric_params_storage_t));
}

esp_err_t config_staging_set_plug_limits(const plug_limits_storage_t *p)
{
    return copy_field(&s_staging.plug_limits, p, sizeof(plug_limits_storage_t));
}

esp_err_t config_staging_set_restart(const restart_params_storage_t *p)
{
    return copy_field(&s_staging.restart, p, sizeof(restart_params_storage_t));
}

esp_err_t config_staging_set_feed(const feed_params_storage_t *p)
{
    return copy_field(&s_staging.feed, p, sizeof(feed_params_storage_t));
}

esp_err_t config_staging_set_security(const security_params_storage_t *p)
{
    return copy_field(&s_staging.security, p, sizeof(security_params_storage_t));
}

esp_err_t config_staging_set_antiflap(const antiflap_params_storage_t *p)
{
    return copy_field(&s_staging.antiflap, p, sizeof(antiflap_params_storage_t));
}

esp_err_t config_staging_set_selftest(const selftest_params_storage_t *p)
{
    return copy_field(&s_staging.selftest, p, sizeof(selftest_params_storage_t));
}

esp_err_t config_staging_set_calibration(const calibration_params_storage_t *p)
{
    return copy_field(&s_staging.calibration, p, sizeof(calibration_params_storage_t));
}

esp_err_t config_staging_commit(void)
{
    if (!s_initialized || !s_dirty) return ESP_OK;
    esp_err_t err;
    err = config_set_thermal(&s_staging.thermal); if (err) return err;
    err = config_set_ato(&s_staging.ato); if (err) return err;
    err = config_set_electric(&s_staging.electric); if (err) return err;
    err = config_set_plug_limits(&s_staging.plug_limits); if (err) return err;
    err = config_set_restart(&s_staging.restart); if (err) return err;
    err = config_set_feed(&s_staging.feed); if (err) return err;
    err = config_set_security(&s_staging.security); if (err) return err;
    err = config_set_antiflap(&s_staging.antiflap); if (err) return err;
    err = config_set_selftest(&s_staging.selftest); if (err) return err;
    err = config_set_calibration(&s_staging.calibration); if (err) return err;
    err = config_save_all(); if (err) return err;
    s_dirty = false;
    return ESP_OK;
}

esp_err_t config_staging_rollback(void)
{
    if (!s_initialized || !s_dirty) return ESP_OK;
    memcpy(&s_staging, &s_backup, sizeof(s_staging));
    s_dirty = false;
    return ESP_OK;
}

bool config_staging_is_dirty(void)
{
    return s_dirty;
}

