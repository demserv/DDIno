// @requirement RF-GLOBAL-005 ViewModel com dados mock centralizados (sem hardcode nas telas)
#include "ui_view_model.h"
#include <string.h>
#include <stdio.h>

void ui_view_model_init_defaults(ui_root_vm_t *vm)
{
    memset(vm, 0, sizeof(ui_root_vm_t));

    /* Topbar */
    vm->topbar.wifi_ok = true;
    snprintf(vm->topbar.datetime_text, sizeof(vm->topbar.datetime_text), "26/06/2026 19:30");
    vm->topbar.feed_mode_active = false;
    vm->topbar.feed_remaining_s = 0;

    /* Footer */
    vm->footer.system_state = UI_SYSTEM_NORMAL;
    snprintf(vm->footer.uptime_text, sizeof(vm->footer.uptime_text), "02h 34m");
    vm->footer.active_alerts_count = 2;
    vm->footer.page_index = 0;
    vm->footer.page_count = 7;

    /* Dashboard */
    vm->dashboard.current_temp_c = 26.5f;
    vm->dashboard.setpoint_temp_c = 25.0f;
    vm->dashboard.thermal_state = UI_THERMAL_IDLE;
    vm->dashboard.voltage_v = 127.2f;
    vm->dashboard.current_a = 1.94f;
    vm->dashboard.power_w = 247.0f;
    vm->dashboard.power_factor = 0.92f;
    vm->dashboard.ato_state = UI_ATO_IDLE;
    snprintf(vm->dashboard.ato_text, sizeof(vm->dashboard.ato_text), "ATO OK");

    /* Devices */
    vm->devices.plug_count = 10;
    {
        /* P01 Aquecedor */
        vm->devices.plugs[0].plug_id = 1;
        snprintf(vm->devices.plugs[0].code, sizeof(vm->devices.plugs[0].code), "P01");
        snprintf(vm->devices.plugs[0].name, sizeof(vm->devices.plugs[0].name), "Aquecedor");
        snprintf(vm->devices.plugs[0].role_tag, sizeof(vm->devices.plugs[0].role_tag), "Termico");
        vm->devices.plugs[0].state = UI_PLUG_ON;
        vm->devices.plugs[0].current_a = 0.85f;
        vm->devices.plugs[0].is_critical = true;
        vm->devices.plugs[0].current_valid = true;

        /* P02 Cooler */
        vm->devices.plugs[1].plug_id = 2;
        snprintf(vm->devices.plugs[1].code, sizeof(vm->devices.plugs[1].code), "P02");
        snprintf(vm->devices.plugs[1].name, sizeof(vm->devices.plugs[1].name), "Cooler");
        snprintf(vm->devices.plugs[1].role_tag, sizeof(vm->devices.plugs[1].role_tag), "Termico");
        vm->devices.plugs[1].state = UI_PLUG_OFF;
        vm->devices.plugs[1].current_a = 0.0f;
        vm->devices.plugs[1].is_critical = true;
        vm->devices.plugs[1].current_valid = true;

        /* P03 Bomba Principal */
        vm->devices.plugs[2].plug_id = 3;
        snprintf(vm->devices.plugs[2].code, sizeof(vm->devices.plugs[2].code), "P03");
        snprintf(vm->devices.plugs[2].name, sizeof(vm->devices.plugs[2].name), "Bomba Principal");
        snprintf(vm->devices.plugs[2].role_tag, sizeof(vm->devices.plugs[2].role_tag), "Circulacao");
        vm->devices.plugs[2].state = UI_PLUG_ON;
        vm->devices.plugs[2].current_a = 0.42f;
        vm->devices.plugs[2].current_valid = true;

        /* P04 Iluminacao */
        vm->devices.plugs[3].plug_id = 4;
        snprintf(vm->devices.plugs[3].code, sizeof(vm->devices.plugs[3].code), "P04");
        snprintf(vm->devices.plugs[3].name, sizeof(vm->devices.plugs[3].name), "Iluminacao LED");
        snprintf(vm->devices.plugs[3].role_tag, sizeof(vm->devices.plugs[3].role_tag), "Iluminacao");
        vm->devices.plugs[3].state = UI_PLUG_ON;
        vm->devices.plugs[3].current_a = 0.31f;
        vm->devices.plugs[3].current_valid = true;

        /* P05 Bomba Skimmer */
        vm->devices.plugs[4].plug_id = 5;
        snprintf(vm->devices.plugs[4].code, sizeof(vm->devices.plugs[4].code), "P05");
        snprintf(vm->devices.plugs[4].name, sizeof(vm->devices.plugs[4].name), "Bomba Skimmer");
        snprintf(vm->devices.plugs[4].role_tag, sizeof(vm->devices.plugs[4].role_tag), "Filtragem");
        vm->devices.plugs[4].state = UI_PLUG_ON;
        vm->devices.plugs[4].current_a = 0.29f;
        vm->devices.plugs[4].current_valid = true;

        /* P06 UV */
        vm->devices.plugs[5].plug_id = 6;
        snprintf(vm->devices.plugs[5].code, sizeof(vm->devices.plugs[5].code), "P06");
        snprintf(vm->devices.plugs[5].name, sizeof(vm->devices.plugs[5].name), "Esterilizador UV");
        snprintf(vm->devices.plugs[5].role_tag, sizeof(vm->devices.plugs[5].role_tag), "Filtragem");
        vm->devices.plugs[5].state = UI_PLUG_OFF;
        vm->devices.plugs[5].current_a = 0.0f;
        vm->devices.plugs[5].current_valid = true;

        /* P07 Bomba Dosadora */
        vm->devices.plugs[6].plug_id = 7;
        snprintf(vm->devices.plugs[6].code, sizeof(vm->devices.plugs[6].code), "P07");
        snprintf(vm->devices.plugs[6].name, sizeof(vm->devices.plugs[6].name), "Bomba Dosadora");
        snprintf(vm->devices.plugs[6].role_tag, sizeof(vm->devices.plugs[6].role_tag), "Dosagem");
        vm->devices.plugs[6].state = UI_PLUG_ON;
        vm->devices.plugs[6].current_a = 0.07f;
        vm->devices.plugs[6].current_valid = true;

        /* P08 Bomba Circulacao */
        vm->devices.plugs[7].plug_id = 8;
        snprintf(vm->devices.plugs[7].code, sizeof(vm->devices.plugs[7].code), "P08");
        snprintf(vm->devices.plugs[7].name, sizeof(vm->devices.plugs[7].name), "Bomba Circulacao");
        snprintf(vm->devices.plugs[7].role_tag, sizeof(vm->devices.plugs[7].role_tag), "Circulacao");
        vm->devices.plugs[7].state = UI_PLUG_ON;
        vm->devices.plugs[7].current_a = 0.18f;
        vm->devices.plugs[7].current_valid = true;

        /* P09 Reserva */
        vm->devices.plugs[8].plug_id = 9;
        snprintf(vm->devices.plugs[8].code, sizeof(vm->devices.plugs[8].code), "P09");
        snprintf(vm->devices.plugs[8].name, sizeof(vm->devices.plugs[8].name), "Reserva A");
        snprintf(vm->devices.plugs[8].role_tag, sizeof(vm->devices.plugs[8].role_tag), "Reserva");
        vm->devices.plugs[8].state = UI_PLUG_OFF;
        vm->devices.plugs[8].current_a = 0.0f;
        vm->devices.plugs[8].current_valid = true;

        /* P10 Reserva */
        vm->devices.plugs[9].plug_id = 10;
        snprintf(vm->devices.plugs[9].code, sizeof(vm->devices.plugs[9].code), "P10");
        snprintf(vm->devices.plugs[9].name, sizeof(vm->devices.plugs[9].name), "Reserva B");
        snprintf(vm->devices.plugs[9].role_tag, sizeof(vm->devices.plugs[9].role_tag), "Reserva");
        vm->devices.plugs[9].state = UI_PLUG_OFF;
        vm->devices.plugs[9].current_a = 0.0f;
        vm->devices.plugs[9].current_valid = true;
    }

    /* Energy */
    vm->energy.voltage_v = 127.2f;
    vm->energy.current_a = 1.94f;
    vm->energy.power_w = 247.0f;
    vm->energy.frequency_hz = 60.0f;
    vm->energy.power_factor = 0.92f;
    {
        const float kwh_values[UI_ENERGY_MONTHS] = {45.2f, 52.8f, 48.1f, 61.3f, 55.0f, 37.6f};
        const char *labels[UI_ENERGY_MONTHS] = {"Jan", "Fev", "Mar", "Abr", "Mai", "Jun"};
        for (int i = 0; i < UI_ENERGY_MONTHS; i++) {
            vm->energy.monthly_kwh[i] = kwh_values[i];
            snprintf(vm->energy.month_labels[i], 8, "%s", labels[i]);
            vm->energy.monthly_data_valid[i] = true;
        }
    }

    /* Alerts */
    {
        /* Active alert 0: ALM-055 CRITICAL - as required by prompt */
        snprintf(vm->alerts.active_alerts[0].id, sizeof(vm->alerts.active_alerts[0].id), "ALM-055");
        vm->alerts.active_alerts[0].severity = UI_SEVERITY_CRITICAL;
        snprintf(vm->alerts.active_alerts[0].severity_text, sizeof(vm->alerts.active_alerts[0].severity_text), "CRITICAL");
        snprintf(vm->alerts.active_alerts[0].message, sizeof(vm->alerts.active_alerts[0].message), "Sobrecarga eletrica detectada");
        snprintf(vm->alerts.active_alerts[0].timestamp, sizeof(vm->alerts.active_alerts[0].timestamp), "19:25");
        vm->alerts.active_alerts[0].acked = false;
        snprintf(vm->alerts.active_alerts[0].action_hint, sizeof(vm->alerts.active_alerts[0].action_hint), "Reduzir consumo eletrico imediatamente");

        /* Active alert 1 */
        snprintf(vm->alerts.active_alerts[1].id, sizeof(vm->alerts.active_alerts[1].id), "ALM-042");
        vm->alerts.active_alerts[1].severity = UI_SEVERITY_WARNING;
        snprintf(vm->alerts.active_alerts[1].severity_text, sizeof(vm->alerts.active_alerts[1].severity_text), "WARNING");
        snprintf(vm->alerts.active_alerts[1].message, sizeof(vm->alerts.active_alerts[1].message), "Consumo elevado no plug P03");
        snprintf(vm->alerts.active_alerts[1].timestamp, sizeof(vm->alerts.active_alerts[1].timestamp), "19:10");
        vm->alerts.active_alerts[1].acked = true;
        snprintf(vm->alerts.active_alerts[1].action_hint, sizeof(vm->alerts.active_alerts[1].action_hint), "Verificar bomba principal");

        vm->alerts.active_count = 2;
        vm->alerts.critical_count = 1;
        vm->alerts.high_count = 0;
        vm->alerts.warning_count = 1;
        vm->alerts.info_count = 0;

        /* History alert */
        snprintf(vm->alerts.history_alerts[0].id, sizeof(vm->alerts.history_alerts[0].id), "ALM-012");
        vm->alerts.history_alerts[0].severity = UI_SEVERITY_HIGH;
        snprintf(vm->alerts.history_alerts[0].severity_text, sizeof(vm->alerts.history_alerts[0].severity_text), "HIGH");
        snprintf(vm->alerts.history_alerts[0].message, sizeof(vm->alerts.history_alerts[0].message), "Temperatura acima do limite");
        snprintf(vm->alerts.history_alerts[0].timestamp, sizeof(vm->alerts.history_alerts[0].timestamp), "18:45");
        vm->alerts.history_alerts[0].acked = true;
        snprintf(vm->alerts.history_alerts[0].action_hint, sizeof(vm->alerts.history_alerts[0].action_hint), "Verificar aquecedor P01");
        vm->alerts.history_count = 1;
    }

    /* Diagnostics */
    vm->diagnostics.temperature = UI_HEALTH_OK;
    vm->diagnostics.ato = UI_HEALTH_OK;
    vm->diagnostics.energy = UI_HEALTH_OK;
    vm->diagnostics.plugs = UI_HEALTH_OK;
    vm->diagnostics.persistence = UI_HEALTH_UNKNOWN;
    vm->diagnostics.security = UI_HEALTH_UNKNOWN;
    vm->diagnostics.selftest = UI_HEALTH_OK;
    vm->diagnostics.buses = UI_HEALTH_OK;
    vm->diagnostics.io = UI_HEALTH_UNKNOWN;

    /* Config temperature */
    vm->config_temperature.setpoint_c = 25.0f;
    vm->config_temperature.temp_min_c = 24.0f;
    vm->config_temperature.temp_max_c = 28.0f;
    vm->config_temperature.temp_critical_c = 32.0f;
    vm->config_temperature.hysteresis_c = 1.0f;
    vm->config_temperature.temp_extreme_c = 35.0f;
}

#include "global_state.h"
#include "alert_manager.h"
#include "config_manager.h"
#include "plug_manager.h"
#include "fsm/thermal_fsm.h"
#include "fsm/ato_fsm.h"
#include "services/electric_fsm.h"
#include "fsm/feed_fsm.h"
#include "health_matrix.h"

extern global_state_t g_gs;
extern thermal_fsm_t g_thermal_fsm;
extern ato_fsm_t g_ato_fsm;
extern electric_fsm_t g_electric_fsm;
extern feed_fsm_t g_feed_fsm;

void ui_view_model_update_from_system(ui_root_vm_t *vm)
{
    if (!vm) return;

    vm->topbar.wifi_ok = g_gs.wifi_ok;
    vm->topbar.feed_mode_active = g_gs.feed_active;
    vm->topbar.feed_remaining_s = g_gs.feed_remaining_s;

    vm->footer.system_state = (ui_system_state_t)g_gs.system_state;
    vm->footer.active_alerts_count = g_gs.active_alerts_count;
    vm->footer.page_index = 0;
    vm->footer.page_count = 1;

    vm->dashboard.current_temp_c = g_gs.temp_filtered_c;
    vm->dashboard.voltage_v = 0;
    vm->dashboard.current_a = 0;
    vm->dashboard.power_w = 0;
    vm->dashboard.power_factor = 0;
}
