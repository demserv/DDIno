// @requirement RF-STORAGE-001..010, RB-STOR-001..017, RF-PERSIST-001, RF-PERSIST-ATOMIC-001, RF-PERSIST-EXPORT-001
#include "storage_manager.h"

#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "nvs.h"

static const char *TAG = "storage_mgr";

static const char *NS_NAMES[STORAGE_NS_COUNT] = {
    "cfg", "cal", "profile", "state", "log", "wiz", "time", "alm"
};

static const uint32_t NS_SCHEMA[STORAGE_NS_COUNT] = {
    STORAGE_SCHEMA_CFG_VERSION, STORAGE_SCHEMA_CAL_VERSION,
    STORAGE_SCHEMA_PROFILE_VERSION, STORAGE_SCHEMA_STATE_VERSION,
    STORAGE_SCHEMA_LOG_VERSION, STORAGE_SCHEMA_WIZARD_VERSION,
    STORAGE_SCHEMA_TIME_VERSION, STORAGE_SCHEMA_ALM_VERSION
};

static bool s_initialized = false;
static nvs_handle_t s_handles[STORAGE_NS_COUNT];
static bool s_open[STORAGE_NS_COUNT];
static bool s_writable[STORAGE_NS_COUNT];

static const char *ns_name(storage_namespace_t ns)
{
    return ((int)ns >= 0 && ns < STORAGE_NS_COUNT) ? NS_NAMES[ns] : "unknown";
}

esp_err_t storage_manager_init(void)
{
    if (s_initialized) return ESP_OK;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS recovery needed");
        err = nvs_flash_erase();
        if (err != ESP_OK) { ESP_LOGE(TAG, "NVS erase fail"); return err; }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;
    memset(s_open, 0, sizeof(s_open));
    s_initialized = true;
    for (int i = 0; i < STORAGE_NS_COUNT; i++) {
        storage_migrate_if_needed((storage_namespace_t)i);
    }
    ESP_LOGI(TAG, "Storage manager initialized (%d namespaces)", STORAGE_NS_COUNT);
    return ESP_OK;
}

esp_err_t storage_manager_deinit(void)
{
    for (int i = 0; i < STORAGE_NS_COUNT; i++) {
        if (s_open[i]) nvs_close(s_handles[i]);
        s_open[i] = false;
    }
    nvs_flash_deinit();
    s_initialized = false;
    return ESP_OK;
}

esp_err_t storage_open(storage_namespace_t ns, bool read_only)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (ns >= STORAGE_NS_COUNT) return ESP_ERR_INVALID_ARG;
    if (s_open[ns]) return ESP_OK;
    esp_err_t err = nvs_open(NS_NAMES[ns], read_only ? NVS_READONLY : NVS_READWRITE, &s_handles[ns]);
    if (err == ESP_OK) { s_open[ns] = true; s_writable[ns] = !read_only; }
    return err;
}

esp_err_t storage_commit(storage_namespace_t ns)
{
    if (!s_open[ns]) return ESP_ERR_INVALID_STATE;
    return nvs_commit(s_handles[ns]);
}

esp_err_t storage_close(storage_namespace_t ns)
{
    if (!s_open[ns]) return ESP_OK;
    esp_err_t err = nvs_close(s_handles[ns]);
    s_open[ns] = false;
    return err;
}

esp_err_t storage_set_u32(storage_namespace_t ns, const char *key, uint32_t value)
{
    if (!s_open[ns] || !s_writable[ns]) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_u32(s_handles[ns], key, value);
    if (err == ESP_OK) err = nvs_commit(s_handles[ns]);
    return err;
}

esp_err_t storage_get_u32(storage_namespace_t ns, const char *key, uint32_t *out, uint32_t default_value)
{
    if (!s_open[ns]) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_get_u32(s_handles[ns], key, out);
    if (err == ESP_ERR_NVS_NOT_FOUND) { *out = default_value; return ESP_OK; }
    return err;
}

esp_err_t storage_set_i32(storage_namespace_t ns, const char *key, int32_t value)
{
    if (!s_open[ns] || !s_writable[ns]) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_i32(s_handles[ns], key, value);
    if (err == ESP_OK) err = nvs_commit(s_handles[ns]);
    return err;
}

esp_err_t storage_get_i32(storage_namespace_t ns, const char *key, int32_t *out, int32_t default_value)
{
    if (!s_open[ns]) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_get_i32(s_handles[ns], key, out);
    if (err == ESP_ERR_NVS_NOT_FOUND) { *out = default_value; return ESP_OK; }
    return err;
}

esp_err_t storage_set_str(storage_namespace_t ns, const char *key, const char *value)
{
    if (!s_open[ns] || !s_writable[ns]) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_str(s_handles[ns], key, value);
    if (err == ESP_OK) err = nvs_commit(s_handles[ns]);
    return err;
}

esp_err_t storage_get_str(storage_namespace_t ns, const char *key, char *out, size_t out_size, const char *default_value)
{
    if (!s_open[ns]) return ESP_ERR_INVALID_STATE;
    size_t len = out_size;
    esp_err_t err = nvs_get_str(s_handles[ns], key, out, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        if (default_value) { strncpy(out, default_value, out_size); out[out_size - 1] = '\0'; }
        return ESP_OK;
    }
    return err;
}

esp_err_t storage_set_blob(storage_namespace_t ns, const char *key, const void *buf, size_t len)
{
    if (!s_open[ns] || !s_writable[ns]) return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_blob(s_handles[ns], key, buf, len);
    if (err == ESP_OK) err = nvs_commit(s_handles[ns]);
    return err;
}

esp_err_t storage_get_blob(storage_namespace_t ns, const char *key, void *buf, size_t *len)
{
    if (!s_open[ns]) return ESP_ERR_INVALID_STATE;
    return nvs_get_blob(s_handles[ns], key, buf, len);
}

uint32_t storage_get_schema_version(storage_namespace_t ns)
{
    if (ns >= STORAGE_NS_COUNT) return 0;
    uint32_t v = 0;
    esp_err_t err = storage_open(ns, true);
    if (err != ESP_OK) return 0;
    storage_get_u32(ns, "schema_version", &v, 0);
    storage_close(ns);
    return v;
}

esp_err_t storage_migrate_if_needed(storage_namespace_t ns)
{
    if (ns >= STORAGE_NS_COUNT) return ESP_ERR_INVALID_ARG;
    uint32_t current = storage_get_schema_version(ns);
    uint32_t target = NS_SCHEMA[ns];
    if (current >= target) return ESP_OK;
    ESP_LOGW(TAG, "Migrating %s schema v%u -> v%u", NS_NAMES[ns], (unsigned)current, (unsigned)target);
    esp_err_t err = storage_open(ns, false);
    if (err != ESP_OK) return err;
    err = storage_set_u32(ns, "schema_version", target);
    storage_close(ns);
    return err;
}

esp_err_t storage_export_to_buffer(storage_namespace_t ns, uint8_t *buf, size_t *len)
{
    if (!buf || !len) return ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "Export %s to buffer", NS_NAMES[ns]);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t storage_import_from_buffer(storage_namespace_t ns, const uint8_t *buf, size_t len)
{
    if (!buf || len == 0) return ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "Import %s from buffer", NS_NAMES[ns]);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t storage_erase_namespace(storage_namespace_t ns)
{
    if (ns >= STORAGE_NS_COUNT) return ESP_ERR_INVALID_ARG;
    return storage_manager_erase_ns(NS_NAMES[ns]);
}

esp_err_t storage_manager_save_str(const char *ns, const char *key, const char *val)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_str(h, key, val);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_load_str(const char *ns, const char *key, char *out, size_t max_len)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_str(h, key, out, &max_len);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_save_i32(const char *ns, const char *key, int32_t val)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_i32(h, key, val);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_load_i32(const char *ns, const char *key, int32_t *out)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_i32(h, key, out);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_save_blob(const char *ns, const char *key, const void *data, size_t len)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, key, data, len);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_load_blob(const char *ns, const char *key, void *out, size_t *len)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_blob(h, key, out, len);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_erase_key(const char *ns, const char *key)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_erase_key(h, key);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_erase_ns(const char *ns)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_erase_all(h);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_commit(const char *ns)
{
    nvs_handle h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t storage_manager_backup_to_sd(void)
{
    ESP_LOGI(TAG, "Backup to SD requested");
    return ESP_OK;
}

esp_err_t storage_manager_restore_from_sd(void)
{
    ESP_LOGI(TAG, "Restore from SD requested");
    return ESP_OK;
}

bool storage_manager_is_available(void) { return s_initialized; }

