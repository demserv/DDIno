// @requirement RF-WIFI-001 a RF-WIFI-015 Gerenciamento de WiFi: Station, AP, scan, credenciais
#ifndef FIRMWARE_INCLUDE_WIFI_CTL_H
#define FIRMWARE_INCLUDE_WIFI_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MODE_NONE = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA
} wifi_ctl_mode_t;

typedef struct {
    char ssid[33];
    char password[65];
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
} wifi_ctl_ap_info_t;

esp_err_t wifi_ctl_init(void);
esp_err_t wifi_ctl_deinit(void);
esp_err_t wifi_ctl_set_mode(wifi_ctl_mode_t mode);
wifi_ctl_mode_t wifi_ctl_get_mode(void);

esp_err_t wifi_ctl_sta_connect(const char *ssid, const char *password);
esp_err_t wifi_ctl_sta_disconnect(void);
bool wifi_ctl_sta_is_connected(void);
int8_t wifi_ctl_sta_get_rssi(void);
char *wifi_ctl_sta_get_ip(void);

esp_err_t wifi_ctl_ap_start(const char *ssid, const char *password, uint8_t channel);
esp_err_t wifi_ctl_ap_stop(void);

esp_err_t wifi_ctl_scan_start(uint16_t timeout_ms);
int wifi_ctl_scan_get_count(void);
wifi_ctl_ap_info_t *wifi_ctl_scan_get_result(int index);
void wifi_ctl_scan_free(void);

esp_err_t wifi_ctl_save_creds(const char *ssid, const char *password);
esp_err_t wifi_ctl_load_creds(char *ssid, size_t ssid_len, char *password, size_t pass_len);
esp_err_t wifi_ctl_clear_creds(void);

#ifdef __cplusplus
}
#endif

#endif
