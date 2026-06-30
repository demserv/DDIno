// @requirement RF-GLOBAL-005 Root ViewModel com dados centralizados
#ifndef HMI_UI_VIEW_MODEL_H
#define HMI_UI_VIEW_MODEL_H

#include "ui_types.h"
#include <stdint.h>
#include <stdbool.h>

#define UI_MAX_PLUGS   10
#define UI_MAX_ALERTS  8
#define UI_ENERGY_MONTHS 6

typedef struct {
    bool wifi_ok;
    char datetime_text[24];
    bool feed_mode_active;
    uint32_t feed_remaining_s;
    bool mute_active;   /* @requirement RF-UI-MUTE-001 Indicação persistente de MUTE */
    /* @requirement RF-UI-STATUS-001 Status bar: SD, manutenção, self-test e pausa do carrossel. */
    bool sd_ok;
    bool maintenance_active;
    bool selftest_failed;
    bool carousel_paused;
} ui_topbar_vm_t;

typedef struct {
    ui_system_state_t system_state;
    char uptime_text[24];
    uint16_t active_alerts_count;
    uint8_t page_index;
    uint8_t page_count;
    bool wizard_active;
    bool time_valid;
} ui_footer_vm_t;

typedef struct {
    float current_temp_c;
    float setpoint_temp_c;
    ui_thermal_state_t thermal_state;
    float voltage_v;
    float current_a;
    float power_w;
    float power_factor;
    ui_ato_state_t ato_state;
    char ato_text[32];
} ui_dashboard_vm_t;

typedef struct {
    uint8_t plug_id;
    char code[4];
    char name[32];
    char role_tag[12];
    ui_plug_state_t state;
    float current_a;
    bool is_critical;
    bool current_valid;
} ui_plug_vm_t;

typedef struct {
    ui_plug_vm_t plugs[UI_MAX_PLUGS];
    uint8_t plug_count;
} ui_devices_vm_t;

typedef struct {
    float voltage_v;
    float current_a;
    float power_w;
    float frequency_hz;
    float power_factor;
    float monthly_kwh[UI_ENERGY_MONTHS];
    char month_labels[UI_ENERGY_MONTHS][8];
    bool monthly_data_valid[UI_ENERGY_MONTHS];
} ui_energy_vm_t;

typedef struct {
    char id[12];
    ui_alert_severity_t severity;
    char severity_text[16];
    char message[80];
    char timestamp[16];
    bool acked;
    char action_hint[96];
} ui_alert_vm_t;

typedef struct {
    ui_alert_vm_t active_alerts[UI_MAX_ALERTS];
    uint8_t active_count;
    ui_alert_vm_t history_alerts[UI_MAX_ALERTS];
    uint8_t history_count;
    uint8_t critical_count;
    uint8_t warning_count;
    uint8_t high_count;
    uint8_t info_count;
} ui_alerts_vm_t;

typedef struct {
    ui_health_state_t temperature;
    ui_health_state_t ato;
    ui_health_state_t energy;
    ui_health_state_t plugs;
    ui_health_state_t persistence;
    ui_health_state_t security;
    ui_health_state_t selftest;
    ui_health_state_t buses;
    ui_health_state_t io;
} ui_diagnostics_vm_t;

typedef struct {
    float setpoint_c;
    float temp_min_c;
    float temp_max_c;
    float temp_critical_c;
    float hysteresis_c;
    float temp_extreme_c;
} ui_config_temperature_vm_t;

typedef struct {
    ui_ato_state_t state;
    int32_t level_adc;
    bool pump_on;
    bool overflow;
} ui_ato_vm_t;

typedef struct {
    ui_topbar_vm_t topbar;
    ui_footer_vm_t footer;
    ui_dashboard_vm_t dashboard;
    ui_devices_vm_t devices;
    ui_energy_vm_t energy;
    ui_alerts_vm_t alerts;
    ui_diagnostics_vm_t diagnostics;
    ui_config_temperature_vm_t config_temperature;
    ui_ato_vm_t ato;
} ui_root_vm_t;

void ui_view_model_init_defaults(ui_root_vm_t *vm);
void ui_view_model_update_from_system(ui_root_vm_t *vm);

#endif
