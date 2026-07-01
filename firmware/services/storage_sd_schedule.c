/* @requirement RF-STORAGE-004 Backup TXT diário (01:00) + log horário no SD
 * Decisão normativa do usuário — 2026-06-30 (itens 1 e 3). */
#include "services/storage_sd.h"
#include "services/config_manager.h"
#include "services/plug_manager.h"
#include "health_matrix.h"
#include "drivers/driver_ph_sensor.h"
#include "drivers/driver_pzem.h"
#include "wifi_ctl.h"
#include "hardware_config.h"
#include "global_state.h"
#include "plug_model.h"

#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern global_state_t g_gs;

static const char *TAG = "sd_sched";

#define SD_MOUNT_POINT       "/sdcard"
#define SD_BACKUP_DIR        SD_MOUNT_POINT "/config/backup"
#define SD_HOURLY_DIR        SD_MOUNT_POINT "/logs/hourly"
#define SD_EVENTS_LOG        SD_MOUNT_POINT "/logs/events/log.txt"
#define SD_BACKUP_PREFIX     "DDINO_BKUP_"
#define SD_BACKUP_SUFFIX     ".txt"

static int s_last_backup_ymd = -1;
static int s_last_hourly_key = -1;

static bool time_ready(void)
{
    time_t now = time(NULL);
    struct tm tm_info;
    if (localtime_r(&now, &tm_info) == NULL) return false;
    return (tm_info.tm_year + 1900) >= 2020;
}

static esp_err_t ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return ESP_OK;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", path);
    (void)system(cmd);
    return ESP_OK;
}

static esp_err_t atomic_write_txt(const char *final_path, const char *tmp_path, FILE *f)
{
    if (!f) return ESP_ERR_INVALID_ARG;
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (rename(tmp_path, final_path) != 0) {
        remove(tmp_path);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static uint32_t crc32_file_content(FILE *f)
{
    uint32_t crc = 0xFFFFFFFFU;
    long pos = ftell(f);
    rewind(f);
    int c;
    while ((c = fgetc(f)) != EOF) {
        crc ^= (uint32_t)c;
        for (int i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)(-(int32_t)(crc & 1U)));
        }
    }
    fseek(f, 0, SEEK_END);
    if (pos >= 0) {
        (void)pos;
    }
    return ~crc;
}

static void rotate_backups(void)
{
    struct dirent *entry;
    DIR *dir = opendir(SD_BACKUP_DIR);
    if (!dir) return;

    char names[HW_SD_BACKUP_RETAIN_COUNT + 2][64];
    time_t mtimes[HW_SD_BACKUP_RETAIN_COUNT + 2];
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < (int)(sizeof(names) / sizeof(names[0]))) {
        if (entry->d_type != DT_REG) continue;
        if (strncmp(entry->d_name, SD_BACKUP_PREFIX, strlen(SD_BACKUP_PREFIX)) != 0) continue;
        if (!strstr(entry->d_name, SD_BACKUP_SUFFIX)) continue;

        char full[320];
        snprintf(full, sizeof(full), "%s/%s", SD_BACKUP_DIR, entry->d_name);
        struct stat st;
        if (stat(full, &st) != 0) continue;

        strncpy(names[count], entry->d_name, sizeof(names[count]) - 1);
        mtimes[count] = st.st_mtime;
        count++;
    }
    closedir(dir);

    while (count >= (int)HW_SD_BACKUP_RETAIN_COUNT) {
        int oldest = 0;
        for (int i = 1; i < count; i++) {
            if (mtimes[i] < mtimes[oldest]) oldest = i;
        }
        char full[320];
        snprintf(full, sizeof(full), "%s/%s", SD_BACKUP_DIR, names[oldest]);
        remove(full);
        ESP_LOGI(TAG, "Backup rotacionado (removido): %s", names[oldest]);
        for (int i = oldest; i < count - 1; i++) {
            strncpy(names[i], names[i + 1], sizeof(names[i]) - 1);
            mtimes[i] = mtimes[i + 1];
        }
        count--;
    }
}

static void append_last_event_logs(FILE *f, int max_lines)
{
    FILE *logf = fopen(SD_EVENTS_LOG, "r");
    if (!logf) {
        fprintf(f, "(sem arquivo de eventos)\n");
        return;
    }

    char lines[10][SD_LOG_LINE_MAX_LEN];
    int count = 0;
    char buf[SD_LOG_LINE_MAX_LEN];

    while (fgets(buf, sizeof(buf), logf)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }
        if (len == 0) continue;
        if (count < max_lines) {
            strncpy(lines[count], buf, SD_LOG_LINE_MAX_LEN - 1);
            lines[count][SD_LOG_LINE_MAX_LEN - 1] = '\0';
            count++;
        } else {
            memmove(lines[0], lines[1], (size_t)(max_lines - 1) * sizeof(lines[0]));
            strncpy(lines[max_lines - 1], buf, SD_LOG_LINE_MAX_LEN - 1);
            lines[max_lines - 1][SD_LOG_LINE_MAX_LEN - 1] = '\0';
        }
    }
    fclose(logf);

    if (count == 0) {
        fprintf(f, "(nenhum log recente)\n");
        return;
    }
    int start = count > max_lines ? count - max_lines : 0;
    for (int i = start; i < count; i++) {
        fprintf(f, "%s\n", lines[i]);
    }
}

static void write_config_section(FILE *f, const char *title, void (*writer)(FILE *))
{
    fprintf(f, "\n[%s]\n", title);
    writer(f);
}

static void write_thermal(FILE *f)
{
    const thermal_params_storage_t *p = config_get_thermal();
    if (!p) return;
    fprintf(f, "temp_normal_c=%.2f\n", p->temp_normal_c);
    fprintf(f, "temp_critical_c=%.2f\n", p->temp_critical_c);
    fprintf(f, "temp_extreme_c=%.2f\n", p->temp_extreme_c);
    fprintf(f, "temp_min_c=%.2f\n", p->temp_min_c);
    fprintf(f, "temp_max_c=%.2f\n", p->temp_max_c);
    fprintf(f, "hysteresis_c=%.2f\n", p->hysteresis_c);
    fprintf(f, "extreme_enabled=%d\n", (int)p->extreme_enabled);
}

static void write_ato(FILE *f)
{
    const ato_params_storage_t *p = config_get_ato();
    if (!p) return;
    fprintf(f, "enabled=%d\n", (int)p->enabled);
    fprintf(f, "low_level_adc=%ld\n", (long)p->low_level_adc);
    fprintf(f, "high_level_adc=%ld\n", (long)p->high_level_adc);
    fprintf(f, "overflow_margin_adc=%ld\n", (long)p->overflow_margin_adc);
    fprintf(f, "refill_timeout_s=%lu\n", (unsigned long)p->refill_timeout_s);
}

static void write_electric(FILE *f)
{
    const electric_params_storage_t *p = config_get_electric();
    if (!p) return;
    fprintf(f, "total_power_limit_w=%.1f\n", p->total_power_limit_w);
    fprintf(f, "per_plug_current_limit_a=%.2f\n", p->per_plug_current_limit_a);
    fprintf(f, "overvoltage_limit_v=%.1f\n", p->overvoltage_limit_v);
    fprintf(f, "undervoltage_limit_v=%.1f\n", p->undervoltage_limit_v);
    fprintf(f, "total_current_limit_a=%.2f\n", p->total_current_limit_a);
    fprintf(f, "fator_curto=%.2f\n", p->fator_curto);
}

static void write_plugs(FILE *f)
{
    for (plug_id_t id = PLUG_ID_P01; id <= PLUG_ID_P10; id++) {
        plug_model_t *p = plug_manager_get(id);
        if (!p) continue;
        fprintf(f, "P%02d nome=%s tipo=%d modo=%d critico=%d lim_a=%.2f\n",
                (int)id, p->name, (int)p->type, (int)p->mode,
                (int)p->is_critical, p->current_limit_a);
    }
}

static void write_network(FILE *f)
{
    char ssid[33] = {0};
    char pass[65] = {0};
    (void)wifi_ctl_load_creds(ssid, sizeof(ssid), pass, sizeof(pass));
    char *ip = wifi_ctl_sta_get_ip();
    fprintf(f, "mode=%d\n", (int)wifi_ctl_get_mode());
    fprintf(f, "ssid=%s\n", ssid[0] ? ssid : "(nao configurado)");
    fprintf(f, "password=(omitida)\n");
    fprintf(f, "ip=%s\n", ip ? ip : "(desconectado)");
    fprintf(f, "rssi=%d\n", (int)wifi_ctl_sta_get_rssi());
    fprintf(f, "connected=%d\n", (int)wifi_ctl_sta_is_connected());
}

static void write_misc_config(FILE *f)
{
    const feed_params_storage_t *feed = config_get_feed();
    const restart_params_storage_t *rst = config_get_restart();
    const plug_limits_storage_t *pl = config_get_plug_limits();
    const system_params_storage_t *sys = config_get_system();
    if (feed) {
        fprintf(f, "feed_duration_min=%lu\n", (unsigned long)feed->feed_duration_min);
        fprintf(f, "feed_cooldown_min=%lu\n", (unsigned long)feed->feed_cooldown_min);
    }
    if (rst) {
        fprintf(f, "restart_espera_s=%lu\n", (unsigned long)rst->tempo_espera_religamento_s);
        fprintf(f, "restart_intervalo_s=%lu\n", (unsigned long)rst->intervalo_religamento_s);
    }
    if (pl) {
        fprintf(f, "min_on_time_s=%lu\n", (unsigned long)pl->min_on_time_s);
        fprintf(f, "min_off_time_s=%lu\n", (unsigned long)pl->min_off_time_s);
    }
    if (sys) {
        fprintf(f, "wizard_completed=%d\n", (int)sys->wizard_completed);
        fprintf(f, "monitor_only=%d\n", (int)sys->monitor_only_mode);
        fprintf(f, "mains_voltage=%u\n", (unsigned)sys->mains_voltage);
    }
}

static esp_err_t write_daily_backup_txt(void)
{
    ensure_dir(SD_BACKUP_DIR);
    rotate_backups();

    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    char final_path[128];
    char tmp_path[132];
    snprintf(final_path, sizeof(final_path),
             "%s/%s%02d_%02d%02d%02d%s",
             SD_BACKUP_DIR, SD_BACKUP_PREFIX,
             (int)((tm_info.tm_mday + tm_info.tm_hour) % HW_SD_BACKUP_RETAIN_COUNT) + 1,
             tm_info.tm_mday, tm_info.tm_mon + 1, tm_info.tm_year % 100,
             SD_BACKUP_SUFFIX);
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", final_path);

    FILE *f = fopen(tmp_path, "w");
    if (!f) return ESP_ERR_NOT_FOUND;

    fprintf(f, "=== DDIno Backup de Configuracao ===\n");
    fprintf(f, "Gerado: %04d-%02d-%02d %02d:%02d:%02d\n",
            tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
            tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
    fprintf(f, "Formato: TXT (decisao normativa usuario 2026-06-30)\n");

    write_config_section(f, "THERMAL", write_thermal);
    write_config_section(f, "ATO", write_ato);
    write_config_section(f, "ELECTRIC", write_electric);
    write_config_section(f, "PLUGS", write_plugs);
    write_config_section(f, "OUTROS", write_misc_config);
    write_config_section(f, "REDE", write_network);

    fprintf(f, "\n[ULTIMOS_10_LOGS]\n");
    append_last_event_logs(f, 10);

    uint32_t crc = crc32_file_content(f);
    fprintf(f, "\n[CRC32]\ncrc32=0x%08lX\n", (unsigned long)crc);

    esp_err_t err = atomic_write_txt(final_path, tmp_path, f);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Backup diario gravado: %s", final_path);
    }
    return err;
}

static void format_sensor_status(char *out, size_t out_len)
{
    const char *temp = health_get(SUB_SENSOR_TEMP) == HEALTH_OK ? "OK" : "FAIL";
    const char *ph = health_get(SUB_SENSOR_PH) == HEALTH_OK ? "OK" :
                     (health_get(SUB_SENSOR_PH) == HEALTH_DEGRADED ? "DEG" : "FAIL");
    const char *lvl = health_get(SUB_SENSOR_LEVEL) == HEALTH_OK ? "OK" : "FAIL";
    const char *cur = health_get(SUB_SENSOR_CURRENT) == HEALTH_OK ? "OK" : "DEG";
    const char *volt = health_get(SUB_SENSOR_VOLTAGE) == HEALTH_OK ? "OK" : "DEG";
    snprintf(out, out_len, "TEMP:%s;PH:%s;LEVEL:%s;CURRENT:%s;VOLT:%s",
             temp, ph, lvl, cur, volt);
}

static esp_err_t write_hourly_log_txt(void)
{
    ensure_dir(SD_HOURLY_DIR);

    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    char path[96];
    snprintf(path, sizeof(path), "%s/LOG%02dde%02d%02d%02d.txt",
             SD_HOURLY_DIR,
             tm_info.tm_hour + 1,
             tm_info.tm_mday,
             tm_info.tm_mon + 1,
             tm_info.tm_year % 100);

    char tmp_path[100];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *f = fopen(tmp_path, "w");
    if (!f) return ESP_ERR_NOT_FOUND;

    char plugs_state[128] = {0};
    char plugs_amps[160] = {0};
    char *ps = plugs_state;
    char *pa = plugs_amps;
    size_t ps_rem = sizeof(plugs_state);
    size_t pa_rem = sizeof(plugs_amps);

    for (plug_id_t id = PLUG_ID_P01; id <= PLUG_ID_P10; id++) {
        plug_model_t *p = plug_manager_get(id);
        if (!p) continue;
        int n = snprintf(ps, ps_rem, "P%02d:%s%s", (int)id,
                         plug_manager_get_effective_state(id) ? "ON" : "OFF",
                         (id < PLUG_ID_P10) ? "," : "");
        if (n > 0 && (size_t)n < ps_rem) { ps += n; ps_rem -= (size_t)n; }
        n = snprintf(pa, pa_rem, "%.2f%s", p->current_a, (id < PLUG_ID_P10) ? "," : "");
        if (n > 0 && (size_t)n < pa_rem) { pa += n; pa_rem -= (size_t)n; }
    }

    pzem_data_t pzem = {0};
    (void)pzem_read_all(&pzem);

    char sensors[96];
    format_sensor_status(sensors, sizeof(sensors));

    fprintf(f, "%04d-%02d-%02d %02d:%02d:%02d;%s;%s;%.1f;%.1f;%s\n",
            tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
            tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
            plugs_state, plugs_amps,
            pzem.valid ? pzem.power_w : 0.0f,
            pzem.valid ? pzem.voltage_v : 0.0f,
            sensors);

    esp_err_t err = atomic_write_txt(path, tmp_path, f);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Log horario gravado: %s", path);
    }
    return err;
}

esp_err_t storage_sd_backup_config_now(void)
{
    if (!storage_sd_is_mounted() || !time_ready()) {
        return ESP_ERR_INVALID_STATE;
    }
    return write_daily_backup_txt();
}

static uint32_t parse_crc_from_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[128];
    uint32_t crc = 0;
    while (fgets(line, sizeof(line), f)) {
        unsigned long val = 0;
        if (sscanf(line, "crc32=0x%lX", &val) == 1 || sscanf(line, "crc32=0x%lx", &val) == 1) {
            crc = (uint32_t)val;
            break;
        }
    }
    fclose(f);
    return crc;
}

bool storage_sd_verify_backup_crc(const char *path, uint32_t *out_crc)
{
    if (!path) return false;
    FILE *f = fopen(path, "r");
    if (!f) return false;

    long footer = -1;
    char line[SD_LOG_LINE_MAX_LEN];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "[CRC32]", 5) == 0) {
            footer = ftell(f);
            break;
        }
    }
    if (footer < 0) {
        fclose(f);
        return false;
    }

    rewind(f);
    uint32_t crc = 0xFFFFFFFFU;
    long pos = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "[CRC32]", 5) == 0) break;
        for (char *p = line; *p; p++) {
            crc ^= (uint8_t)*p;
            for (int i = 0; i < 8; i++) {
                crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)(-(int32_t)(crc & 1U)));
            }
        }
        pos = ftell(f);
        (void)pos;
    }
    crc = ~crc;
    fclose(f);

    uint32_t stored = parse_crc_from_file(path);
    if (out_crc) *out_crc = crc;
    return (stored != 0 && stored == crc);
}

void storage_sd_tick_schedules(void)
{
    if (!storage_sd_is_mounted() || !time_ready()) return;

    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    int ymd = (tm_info.tm_year + 1900) * 10000 + (tm_info.tm_mon + 1) * 100 + tm_info.tm_mday;
    int hour_key = ymd * 100 + tm_info.tm_hour;

    if (tm_info.tm_hour == (int)HW_SD_BACKUP_HOUR &&
        tm_info.tm_min == (int)HW_SD_BACKUP_MINUTE &&
        s_last_backup_ymd != ymd) {
        if (write_daily_backup_txt() == ESP_OK) {
            s_last_backup_ymd = ymd;
        }
    }

    if (tm_info.tm_min == 0 && s_last_hourly_key != hour_key) {
        if (write_hourly_log_txt() == ESP_OK) {
            s_last_hourly_key = hour_key;
        }
    }
}
