// @requirement RF-FLOW-BOOT-004 .tmp orphan handling
// @requirement RF-STORAGE-002 RAM fallback quando SD ausente
// @requirement RF-STORAGE-003 Escrita atômica
#include "services/storage_sd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "pin_map.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "global_state.h"
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>


extern global_state_t g_gs;

static const char *TAG = "storage_sd";

#define SD_MOUNT_POINT "/sdcard"
#define SD_SPI_HOST    SPI2_HOST

static bool s_mounted = false;

static char s_ram_fallback_buf[SD_RAM_FALLBACK_COUNT][SD_LOG_LINE_MAX_LEN];
static uint32_t s_ram_fallback_head = 0;
static uint32_t s_ram_fallback_count = 0;

static const char *log_type_to_dir(sd_log_type_t type)
{
    switch (type) {
        case SD_LOG_TYPE_EVENT:  return "logs/events";
        case SD_LOG_TYPE_ALERT:  return "logs/alerts";
        case SD_LOG_TYPE_ENERGY: return "logs/energy";
        case SD_LOG_TYPE_AUDIT:  return "logs/security";
        default:                 return "logs/misc";
    }
}

static esp_err_t ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return ESP_OK;
    }
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", path);
    int ret = system(cmd);
    (void)ret;
    return ESP_OK;
}

static esp_err_t ensure_all_dirs(void)
{
    ensure_dir(SD_MOUNT_POINT "/logs/events");
    ensure_dir(SD_MOUNT_POINT "/logs/alerts");
    ensure_dir(SD_MOUNT_POINT "/logs/energy");
    ensure_dir(SD_MOUNT_POINT "/logs/security");
    ensure_dir(SD_MOUNT_POINT "/config/backup");
    ensure_dir(SD_MOUNT_POINT "/config/profiles");
    ensure_dir(SD_MOUNT_POINT "/system");
    return ESP_OK;
}

static esp_err_t atomic_write_and_rename(const char *final_path, const char *tmp_path, FILE *f)
{
    if (!f) return ESP_ERR_INVALID_ARG;
    fclose(f);

    if (rename(tmp_path, final_path) != 0) {
        ESP_LOGE(TAG, "Falha ao renomear %s -> %s", tmp_path, final_path);
        remove(tmp_path);
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t storage_sd_init(void)
{
    if (s_mounted) return ESP_OK;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.host_id = SD_SPI_HOST;
    slot_cfg.gpio_cs = PIN_SD_CS_GPIO;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    esp_err_t err = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_cfg, &mount_cfg, &card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD mount falhou: %s", esp_err_to_name(err));
        s_mounted = false;
        return err;
    }

    s_mounted = true;
    ensure_all_dirs();

    const char *known_dirs[] = {
        SD_MOUNT_POINT "/logs/events",
        SD_MOUNT_POINT "/logs/alerts",
        SD_MOUNT_POINT "/logs/energy",
        SD_MOUNT_POINT "/logs/security",
        SD_MOUNT_POINT "/config/backup",
        SD_MOUNT_POINT "/config/profiles",
        SD_MOUNT_POINT "/system"
    };
    for (int d = 0; d < (int)(sizeof(known_dirs) / sizeof(known_dirs[0])); d++) {
        DIR *dir = opendir(known_dirs[d]);
        if (!dir) continue;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_REG) continue;
            size_t nlen = strlen(entry->d_name);
            if (nlen < 5) continue;
            if (strcmp(entry->d_name + nlen - 4, ".tmp") != 0) continue;
            char fullpath[320];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", known_dirs[d], entry->d_name);
            char origpath[320];
            snprintf(origpath, sizeof(origpath), "%s/%.*s", known_dirs[d], (int)(nlen - 4), entry->d_name);
            FILE *f = fopen(fullpath, "r");
            if (f) {
                int c = fgetc(f);
                fclose(f);
                if (c == '{') {
                    if (rename(fullpath, origpath) == 0) {
                        ESP_LOGI(TAG, "Recovered .tmp file -> %s", origpath);
                    } else {
                        ESP_LOGW(TAG, "Could not rename .tmp file %s", fullpath);
                        remove(fullpath);
                    }
                } else {
                    remove(fullpath);
                    ESP_LOGW(TAG, "Removed invalid .tmp file %s", fullpath);
                }
            } else {
                remove(fullpath);
                ESP_LOGW(TAG, "Removed unreadable .tmp file %s", fullpath);
            }
        }
        closedir(dir);
    }

    ESP_LOGI(TAG, "SD montado em %s, tamanho=%lluMB", SD_MOUNT_POINT,
             (unsigned long long)(card->csd.capacity) / 1024);

    storage_sd_flush_ram_fallback();

    return ESP_OK;
}

bool storage_sd_is_mounted(void)
{
    return s_mounted;
}

esp_err_t storage_sd_write_log(sd_log_type_t type, const char *line)
{
    if (!line) return ESP_ERR_INVALID_STATE;

    if (!s_mounted) {
        uint32_t idx = s_ram_fallback_head % SD_RAM_FALLBACK_COUNT;
        strncpy(s_ram_fallback_buf[idx], line, SD_LOG_LINE_MAX_LEN - 1);
        s_ram_fallback_buf[idx][SD_LOG_LINE_MAX_LEN - 1] = '\0';
        s_ram_fallback_head++;
        if (s_ram_fallback_count < SD_RAM_FALLBACK_COUNT) {
            s_ram_fallback_count++;
        }
        return ESP_OK;
    }

    char full_dir[64];
    snprintf(full_dir, sizeof(full_dir), "%s/%s", SD_MOUNT_POINT, log_type_to_dir(type));

    char path[96];
    snprintf(path, sizeof(path), "%s/log.txt", full_dir);

    char tmp_path[100];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *f = fopen(tmp_path, "a");
    if (!f) return ESP_ERR_NOT_FOUND;

    fprintf(f, "%s\n", line);
    return atomic_write_and_rename(path, tmp_path, f);
}

void storage_sd_flush_ram_fallback(void)
{
    if (!s_mounted || s_ram_fallback_count == 0) return;

    uint32_t start = (s_ram_fallback_head >= s_ram_fallback_count)
        ? (s_ram_fallback_head - s_ram_fallback_count) : 0;
    ESP_LOGI(TAG, "Flushing %lu RAM fallback entries to SD", (unsigned long)s_ram_fallback_count);

    for (uint32_t i = 0; i < s_ram_fallback_count; i++) {
        uint32_t idx = (start + i) % SD_RAM_FALLBACK_COUNT;
        if (s_ram_fallback_buf[idx][0] == '\0') continue;
        storage_sd_write_log(SD_LOG_TYPE_EVENT, s_ram_fallback_buf[idx]);
        s_ram_fallback_buf[idx][0] = '\0';
    }
    s_ram_fallback_head = 0;
    s_ram_fallback_count = 0;
}

uint32_t storage_sd_ram_fallback_count(void)
{
    return s_ram_fallback_count;
}

esp_err_t storage_sd_write_json_atomic(const char *filename, const char *json_content)
{
    if (!s_mounted || !filename || !json_content) return ESP_ERR_INVALID_STATE;

    char path[128];
    snprintf(path, sizeof(path), "%s/%s", SD_MOUNT_POINT, filename);

    char tmp_path[132];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *f = fopen(tmp_path, "w");
    if (!f) return ESP_ERR_NOT_FOUND;

    fprintf(f, "%s\n", json_content);
    return atomic_write_and_rename(path, tmp_path, f);
}

esp_err_t storage_sd_backup_config(void)
{
    if (!s_mounted) return ESP_ERR_INVALID_STATE;

    char tmp_path[128];
    snprintf(tmp_path, sizeof(tmp_path), "%s/config/backup/config_backup.json.tmp", SD_MOUNT_POINT);

    FILE *f = fopen(tmp_path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Falha ao criar backup temp");
        return ESP_ERR_NOT_FOUND;
    }

    fprintf(f, "{\"type\":\"config_backup\",\"ts\":%llu,\"state\":%d,\"alerts\":%d}\n",
        (unsigned long long)(esp_timer_get_time() / 1000ULL),
        g_gs.system_state, g_gs.active_alerts_count);

    char final_path[128];
    snprintf(final_path, sizeof(final_path), "%s/config/backup/config_backup.json", SD_MOUNT_POINT);

    return atomic_write_and_rename(final_path, tmp_path, f);
}

esp_err_t storage_sd_unmount(void)
{
    if (!s_mounted) return ESP_OK;

    esp_err_t err = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, NULL);
    if (err == ESP_OK) {
        s_mounted = false;
        ESP_LOGI(TAG, "SD desmontado");
    }
    return err;
}
