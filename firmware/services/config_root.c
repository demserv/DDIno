#include "config_root.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "cfg_root";

static const uint32_t *crc32_table(void)
{
    static uint32_t table[256];
    static bool built = false;
    if (!built) {
        const uint32_t poly = 0xEDB88320;
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (int j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ poly;
                } else {
                    crc >>= 1;
                }
            }
            table[i] = crc;
        }
        built = true;
    }
    return table;
}

static uint32_t calc_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t *table = crc32_table();
    for (size_t i = 0; i < len; i++) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

esp_err_t config_root_compute_crc(config_root_t *root)
{
    if (!root) return ESP_ERR_INVALID_ARG;
    root->crc32 = 0;
    size_t len = sizeof(config_root_t);
    root->crc32 = calc_crc32((const uint8_t *)root, len);
    return ESP_OK;
}

bool config_root_validate(const config_root_t *root)
{
    if (!root) return false;
    uint32_t stored = root->crc32;
    config_root_t tmp = *root;
    tmp.crc32 = 0;
    size_t len = sizeof(config_root_t);
    uint32_t computed = calc_crc32((const uint8_t *)&tmp, len);
    if (stored != computed) {
        ESP_LOGW(TAG, "CRC mismatch: stored=0x%08"PRIX32" computed=0x%08"PRIX32"", stored, computed);
        return false;
    }
    if (root->schema_version[0] != CONFIG_ROOT_SCHEMA_VERSION[0]) {
        ESP_LOGW(TAG, "Schema major mismatch: stored=%s current=%s",
                 root->schema_version, CONFIG_ROOT_SCHEMA_VERSION);
        return false;
    }
    return true;
}

static esp_err_t root_nvs_write(const config_root_t *root)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(CONFIG_ROOT_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, CONFIG_ROOT_NVS_KEY, root, sizeof(config_root_t));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

static esp_err_t root_nvs_read(config_root_t *root)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(CONFIG_ROOT_NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    size_t sz = sizeof(config_root_t);
    err = nvs_get_blob(h, CONFIG_ROOT_NVS_KEY, root, &sz);
    nvs_close(h);
    if (err != ESP_OK) return err;
    if (sz != sizeof(config_root_t)) return ESP_ERR_NVS_NOT_FOUND;
    return ESP_OK;
}

esp_err_t config_root_load(config_root_t *root)
{
    if (!root) return ESP_ERR_INVALID_ARG;
    esp_err_t err = root_nvs_read(root);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No ConfigRoot in NVS, need init: %s", esp_err_to_name(err));
        return err;
    }
    if (!config_root_validate(root)) {
        ESP_LOGW(TAG, "ConfigRoot CRC/version invalid");
        return ESP_ERR_INVALID_CRC;
    }
    ESP_LOGI(TAG, "ConfigRoot loaded OK (CRC=0x%08"PRIX32")", root->crc32);
    return ESP_OK;
}

esp_err_t config_root_save(const config_root_t *root)
{
    if (!root) return ESP_ERR_INVALID_ARG;
    config_root_t writable = *root;
    config_root_compute_crc(&writable);
    esp_err_t err = root_nvs_write(&writable);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "ConfigRoot saved OK (CRC=0x%08"PRIX32")", writable.crc32);
    }
    return err;
}

esp_err_t config_root_commit(config_root_t *root)
{
    return config_root_save(root);
}

esp_err_t config_root_rollback(config_root_t *root)
{
    if (!root) return ESP_ERR_INVALID_ARG;
    esp_err_t err = config_root_load(root);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Rollback: no prior root to restore");
        memset(root, 0, sizeof(config_root_t));
    }
    return err;
}

