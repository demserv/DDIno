// @requirement RF-WIFI-001 a RF-WIFI-015 Gerenciamento WiFi
#include "wifi_ctl.h"

#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *TAG = "wifi_ctl";
static bool s_initialized = false;
static wifi_ctl_mode_t s_mode = WIFI_CTL_MODE_NONE;
static esp_netif_t *s_netif_sta = NULL;
static esp_netif_t *s_netif_ap = NULL;

esp_err_t wifi_ctl_init(void)
{
    if (s_initialized) return ESP_OK;
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) return err;
    s_netif_sta = esp_netif_create_default_wifi_sta();
    s_netif_ap = esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err == ESP_OK) {
        s_initialized = true;
        ESP_LOGI(TAG, "WiFi initialized");
    }
    return err;
}

esp_err_t wifi_ctl_deinit(void)
{
    if (!s_initialized) return ESP_OK;
    esp_wifi_stop();
    esp_wifi_deinit();
    s_initialized = false;
    s_mode = WIFI_CTL_MODE_NONE;
    return ESP_OK;
}

esp_err_t wifi_ctl_set_mode(wifi_ctl_mode_t mode)
{
    wifi_mode_t wm;
    switch (mode) {
        case WIFI_CTL_MODE_STA:   wm = WIFI_MODE_STA; break;
        case WIFI_CTL_MODE_AP:    wm = WIFI_MODE_AP; break;
        case WIFI_CTL_MODE_APSTA: wm = WIFI_MODE_APSTA; break;
        default:              wm = WIFI_MODE_NULL; break;
    }
    esp_err_t err = esp_wifi_set_mode(wm);
    if (err == ESP_OK) s_mode = mode;
    return err;
}

wifi_ctl_mode_t wifi_ctl_get_mode(void) { return s_mode; }

esp_err_t wifi_ctl_sta_connect(const char *ssid, const char *password)
{
    wifi_config_t cfg = {0};
    strncpy((char *)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid) - 1);
    strncpy((char *)cfg.sta.password, password, sizeof(cfg.sta.password) - 1);
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if (err != ESP_OK) return err;
    return esp_wifi_connect();
}

esp_err_t wifi_ctl_sta_disconnect(void) { return esp_wifi_disconnect(); }
bool wifi_ctl_sta_is_connected(void)
{
    wifi_ap_record_t ap;
    return (esp_wifi_sta_get_ap_info(&ap) == ESP_OK);
}

int8_t wifi_ctl_sta_get_rssi(void)
{
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) return ap.rssi;
    return -127;
}

char *wifi_ctl_sta_get_ip(void)
{
    static char ip_str[16] = "0.0.0.0";
    if (!s_netif_sta) return ip_str;
    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(s_netif_sta, &ip) == ESP_OK) {
        uint8_t *b = (uint8_t *)&ip.ip.addr;
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
    }
    return ip_str;
}

esp_err_t wifi_ctl_ap_start(const char *ssid, const char *password, uint8_t channel)
{
    wifi_config_t cfg = {0};
    strncpy((char *)cfg.ap.ssid, ssid, sizeof(cfg.ap.ssid) - 1);
    cfg.ap.ssid_len = strlen(ssid);
    cfg.ap.channel = channel;
    cfg.ap.max_connection = 4;
    if (password && strlen(password) > 0) {
        strncpy((char *)cfg.ap.password, password, sizeof(cfg.ap.password) - 1);
        cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        cfg.ap.authmode = WIFI_AUTH_OPEN;
    }
    return esp_wifi_set_config(WIFI_IF_AP, &cfg);
}

esp_err_t wifi_ctl_ap_stop(void) { return esp_wifi_stop(); }

esp_err_t wifi_ctl_scan_start(uint16_t timeout_ms)
{
    wifi_scan_config_t sc = {0};
    sc.show_hidden = false;
    sc.scan_type = WIFI_SCAN_TYPE_PASSIVE;
    sc.scan_time.passive = timeout_ms;
    return esp_wifi_scan_start(&sc, true);
}

int wifi_ctl_scan_get_count(void)
{
    uint16_t count = 0;
    esp_wifi_scan_get_ap_num(&count);
    return (int)count;
}

wifi_ctl_ap_info_t *wifi_ctl_scan_get_result(int index)
{
    static wifi_ctl_ap_info_t info;
    wifi_ap_record_t records[10];
    uint16_t count = 10;
    if (esp_wifi_scan_get_ap_records(&count, records) != ESP_OK) return NULL;
    if (index >= (int)count) return NULL;
    strncpy(info.ssid, (char *)records[index].ssid, sizeof(info.ssid) - 1);
    info.rssi = records[index].rssi;
    info.channel = records[index].primary;
    memcpy(info.bssid, records[index].bssid, 6);
    return &info;
}

void wifi_ctl_scan_free(void) { esp_wifi_scan_stop(); }

esp_err_t wifi_ctl_save_creds(const char *ssid, const char *password) { return ESP_OK; }
esp_err_t wifi_ctl_load_creds(char *ssid, size_t ssid_len, char *password, size_t pass_len) { return ESP_ERR_NOT_FOUND; }
esp_err_t wifi_ctl_clear_creds(void) { return ESP_OK; }

