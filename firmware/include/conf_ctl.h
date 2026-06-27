// @requirement RF-CONF-001 a RF-CONF-020 Gerenciamento estendido de configuração
#ifndef FIRMWARE_INCLUDE_CONF_CTL_H
#define FIRMWARE_INCLUDE_CONF_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONF_PROFILE_DEFAULT = 0,
    CONF_PROFILE_ECO,
    CONF_PROFILE_HIGH_PERF,
    CONF_PROFILE_CUSTOM
} conf_ctl_profile_t;

typedef struct {
    char ssid[33];
    char password[65];
    bool dhcp_enabled;
    char static_ip[16];
    char gateway[16];
    char netmask[16];
} conf_ctl_network_t;

typedef struct {
    float temp_min_c;
    float temp_max_c;
    float temp_alarm_c;
} conf_ctl_sensor_t;

typedef struct {
    uint32_t relay_on_delay_ms;
    uint32_t relay_off_delay_ms;
    bool emergency_off_enabled;
} conf_ctl_actuator_t;

typedef struct {
    bool alert_audio_enabled;
    uint32_t ack_timeout_s;
    uint32_t spam_cooldown_ms;
} conf_ctl_alert_t;

typedef struct {
    uint8_t brightness;
    bool auto_sleep;
    uint32_t sleep_timeout_s;
} conf_ctl_display_t;

typedef struct {
    log_ctl_level_t level;
    bool sd_logging;
    uint32_t max_file_size_kb;
} conf_ctl_log_t;

typedef struct {
    bool ota_enabled;
    char ota_server_url[256];
    uint32_t ota_check_interval_h;
} conf_ctl_ota_t;

typedef struct {
    char timezone[64];
    int8_t utc_offset_h;
    bool ntp_enabled;
    char ntp_server[128];
} conf_ctl_datetime_t;

typedef struct {
    char language[8];
    char temp_unit; /* 'C' or 'F' */
} conf_ctl_locale_t;

typedef struct {
    conf_ctl_network_t network;
    conf_ctl_sensor_t sensor;
    conf_ctl_actuator_t actuator;
    conf_ctl_alert_t alert;
    conf_ctl_display_t display;
    conf_ctl_log_t log;
    conf_ctl_ota_t ota;
    conf_ctl_datetime_t datetime;
    conf_ctl_locale_t locale;
} conf_ctl_config_t;

const char *conf_ctl_get_name(void);
esp_err_t conf_ctl_init(void);
esp_err_t conf_ctl_load(conf_ctl_config_t *cfg);
esp_err_t conf_ctl_save(const conf_ctl_config_t *cfg);
esp_err_t conf_ctl_validate(const conf_ctl_config_t *cfg);
esp_err_t conf_ctl_rollback(void);
esp_err_t conf_ctl_export_json(char *buf, size_t buf_size);
esp_err_t conf_ctl_import_json(const char *json);
esp_err_t conf_ctl_set_profile(conf_ctl_profile_t profile);
conf_ctl_profile_t conf_ctl_get_profile(void);

#ifdef __cplusplus
}
#endif

#endif
