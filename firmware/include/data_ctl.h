// @requirement DATA — Gerenciamento centralizado de dados: snapshots, export, limpeza
#ifndef FIRMWARE_INCLUDE_DATA_CTL_H
#define FIRMWARE_INCLUDE_DATA_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperature_c;
    float current_a;
    float power_w;
    float energy_kwh;
    float ph;
    float tds_ppm;
    uint32_t uptime_s;
    int8_t wifi_rssi;
} data_ctl_snapshot_t;

esp_err_t data_ctl_init(void);
esp_err_t data_ctl_take_snapshot(data_ctl_snapshot_t *out);
esp_err_t data_ctl_export_snapshot(char *buf, size_t buf_size);
esp_err_t data_ctl_clear_history(void);
int data_ctl_get_history_count(void);
esp_err_t data_ctl_purge_older_than(uint32_t age_hours);

#ifdef __cplusplus
}
#endif

#endif
