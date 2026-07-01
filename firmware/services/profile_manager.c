// @requirement RF-UI-MENU-002 / RF-PERSIST-PROFILE-001 CRUD de perfis (SD + índice NVS)
#include "profile_manager.h"
#include "config_export.h"
#include "config_manager.h"
#include "storage_sd.h"

#include "cJSON.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "prof_mgr";
static const char *NVS_NS = "profiles";
static const char *NVS_KEY = "index";

static bool name_valid(const char *name)
{
    if (!name || name[0] == '\0') return false;
    size_t len = strlen(name);
    if (len >= PROFILE_NAME_MAX) return false;
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

static esp_err_t read_index(char names[][PROFILE_NAME_MAX], uint8_t *count)
{
    if (!names || !count) return ESP_ERR_INVALID_ARG;
    *count = 0;
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    size_t sz = PROFILE_MANAGER_MAX * PROFILE_NAME_MAX;
    err = nvs_get_blob(h, NVS_KEY, names, &sz);
    nvs_close(h);
    if (err == ESP_OK) {
        *count = (uint8_t)(sz / PROFILE_NAME_MAX);
        if (*count > PROFILE_MANAGER_MAX) *count = PROFILE_MANAGER_MAX;
    }
    return err;
}

static esp_err_t write_index(const char names[][PROFILE_NAME_MAX], uint8_t count)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, NVS_KEY, names, (size_t)count * PROFILE_NAME_MAX);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

static void profile_path(const char *name, char *out, size_t out_len)
{
    snprintf(out, out_len, "/sdcard/config/profiles/%s.json", name);
}

bool profile_manager_is_name_valid(const char *name)
{
    return name_valid(name);
}

esp_err_t profile_manager_init(void)
{
    ESP_LOGI(TAG, "Profile manager ready");
    return ESP_OK;
}

esp_err_t profile_manager_list(char names[][PROFILE_NAME_MAX], uint8_t *count)
{
    esp_err_t err = read_index(names, count);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *count = 0;
        return ESP_OK;
    }
    return err;
}

esp_err_t profile_manager_save(const char *name)
{
    if (!name_valid(name)) return ESP_ERR_INVALID_ARG;
    if (!storage_sd_is_mounted()) return ESP_ERR_INVALID_STATE;

    cJSON *json = NULL;
    esp_err_t err = config_export_to_json(&json);
    if (err != ESP_OK || !json) return err;

    char *str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!str) return ESP_ERR_NO_MEM;

    char path[96];
    profile_path(name, path, sizeof(path));

    FILE *f = fopen(path, "w");
    if (!f) {
        free(str);
        return ESP_FAIL;
    }
    fputs(str, f);
    fclose(f);
    free(str);

    char idx[PROFILE_MANAGER_MAX][PROFILE_NAME_MAX];
    uint8_t count = 0;
    read_index(idx, &count);

    int slot = -1;
    for (uint8_t i = 0; i < count; i++) {
        if (strcmp(idx[i], name) == 0) {
            slot = (int)i;
            break;
        }
    }
    if (slot < 0) {
        if (count >= PROFILE_MANAGER_MAX) return ESP_ERR_NO_MEM;
        slot = (int)count;
        snprintf(idx[slot], PROFILE_NAME_MAX, "%s", name);
        count++;
    }

    err = write_index(idx, count);
    ESP_LOGI(TAG, "Profile saved: %s", name);
    return err;
}

esp_err_t profile_manager_load(const char *name)
{
    if (!name_valid(name)) return ESP_ERR_INVALID_ARG;
    if (!storage_sd_is_mounted()) return ESP_ERR_INVALID_STATE;

    char path[96];
    profile_path(name, path, sizeof(path));

    struct stat st;
    if (stat(path, &st) != 0) return ESP_ERR_NOT_FOUND;

    FILE *f = fopen(path, "r");
    if (!f) return ESP_FAIL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 65536) {
        fclose(f);
        return ESP_ERR_INVALID_SIZE;
    }
    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';

    cJSON *json = cJSON_Parse(buf);
    free(buf);
    if (!json) return ESP_ERR_INVALID_ARG;

    bool valid = false;
    uint32_t crc = 0;
    esp_err_t err = config_import_from_json(json, false, &valid, &crc);
    cJSON_Delete(json);
    if (err != ESP_OK || !valid) return ESP_FAIL;

    ESP_LOGI(TAG, "Profile loaded: %s (crc=0x%08" PRIX32 ")", name, crc);
    return ESP_OK;
}

esp_err_t profile_manager_rename(const char *old_name, const char *new_name)
{
    if (!name_valid(old_name) || !name_valid(new_name)) return ESP_ERR_INVALID_ARG;
    if (!storage_sd_is_mounted()) return ESP_ERR_INVALID_STATE;

    char old_path[96], new_path[96];
    profile_path(old_name, old_path, sizeof(old_path));
    profile_path(new_name, new_path, sizeof(new_path));

    if (rename(old_path, new_path) != 0) return ESP_FAIL;

    char idx[PROFILE_MANAGER_MAX][PROFILE_NAME_MAX];
    uint8_t count = 0;
    esp_err_t err = read_index(idx, &count);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    for (uint8_t i = 0; i < count; i++) {
        if (strcmp(idx[i], old_name) == 0) {
            snprintf(idx[i], PROFILE_NAME_MAX, "%s", new_name);
            break;
        }
    }
    return write_index(idx, count);
}

esp_err_t profile_manager_delete(const char *name)
{
    if (!name_valid(name)) return ESP_ERR_INVALID_ARG;

    char path[96];
    profile_path(name, path, sizeof(path));
    remove(path);

    char idx[PROFILE_MANAGER_MAX][PROFILE_NAME_MAX];
    uint8_t count = 0;
    read_index(idx, &count);

    uint8_t w = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (strcmp(idx[i], name) != 0) {
            if (w != i) memcpy(idx[w], idx[i], PROFILE_NAME_MAX);
            w++;
        }
    }
    return write_index(idx, w);
}
