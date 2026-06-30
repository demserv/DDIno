// @requirement RF-GLOBAL-005 ViewModel centralizado — UI consome dados reais do sistema
#include "ui_view_model.h"

#include "ui_screen_manager.h"
#include "global_state.h"
#include "alert_manager.h"
#include "config_manager.h"
#include "plug_manager.h"
#include "health_matrix.h"
#include "time_manager.h"
#include "driver_pzem.h"
#include "plug_model.h"
#include "alert_model.h"
#include "ato_service.h"
#include "driver_buzzer_led.h"
#include "esp_err.h"
#include <time.h>
#include <string.h>

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

static ui_health_state_t map_health(health_status_t st)
{
    switch (st) {
        case HEALTH_OK:       return UI_HEALTH_OK;
        case HEALTH_DEGRADED: return UI_HEALTH_DEGRADED;
        case HEALTH_FAILED:   return UI_HEALTH_FAILED;
        default:              return UI_HEALTH_UNKNOWN;
    }
}

static ui_plug_state_t map_plug_state(plug_effective_state_t st)
{
    switch (st) {
        case PLUG_EFFECTIVE_STATE_ON:       return UI_PLUG_ON;
        case PLUG_EFFECTIVE_STATE_BLOCKED:  return UI_PLUG_BLOCKED;
        case PLUG_EFFECTIVE_STATE_FAULT:
        case PLUG_EFFECTIVE_STATE_UNAVAILABLE:
            return UI_PLUG_ERROR;
        default:                            return UI_PLUG_OFF;
    }
}

static ui_alert_severity_t map_alert_severity(alert_severity_t sev)
{
    switch (sev) {
        case ALERT_SEVERITY_WARNING:  return UI_SEVERITY_WARNING;
        case ALERT_SEVERITY_HIGH:     return UI_SEVERITY_HIGH;
        case ALERT_SEVERITY_CRITICAL: return UI_SEVERITY_CRITICAL;
        default:                      return UI_SEVERITY_INFO;
    }
}

static ui_thermal_state_t map_thermal_from_gs(void)
{
    if (!g_gs.temp_ok) {
        return UI_THERMAL_CRITICAL;
    }
    const thermal_params_storage_t *tp = config_get_thermal();
    float t = g_gs.temp_filtered_c;
    if (t >= tp->temp_extreme_c && tp->extreme_enabled) {
        return UI_THERMAL_CRITICAL;
    }
    if (t >= tp->temp_critical_c) {
        return UI_THERMAL_CRITICAL;
    }
    if (t > tp->temp_normal_c + tp->hysteresis_c) {
        return UI_THERMAL_ALERT;
    }
    if (t < tp->temp_normal_c - tp->hysteresis_c) {
        return UI_THERMAL_COOLING;
    }
    return UI_THERMAL_IDLE;
}

static ui_ato_state_t map_ato_from_gs(void)
{
    if (!g_gs.ato_ok) {
        return UI_ATO_ERROR;
    }
    return UI_ATO_IDLE;
}

static void format_uptime(uint64_t uptime_s, char *buf, size_t len)
{
    uint64_t h = uptime_s / 3600ULL;
    uint64_t m = (uptime_s % 3600ULL) / 60ULL;
    snprintf(buf, len, "%02llu h %02llu m", (unsigned long long)h, (unsigned long long)m);
}

void ui_view_model_init_defaults(ui_root_vm_t *vm)
{
    if (!vm) {
        return;
    }
    memset(vm, 0, sizeof(*vm));
    vm->footer.system_state = UI_SYSTEM_NORMAL;
    vm->footer.page_count = 4;
    vm->devices.plug_count = UI_MAX_PLUGS;
}

void ui_view_model_update_from_system(ui_root_vm_t *vm)
{
    if (!vm) {
        return;
    }

    vm->topbar.wifi_ok = g_gs.wifi_ok;
    vm->topbar.feed_mode_active = g_gs.feed_active;
    vm->topbar.feed_remaining_s = g_gs.feed_remaining_s;
    vm->topbar.mute_active = buzzer_is_muted();
    /* @requirement RF-UI-STATUS-001 Demais indicadores da status bar. */
    vm->topbar.sd_ok = g_gs.sd_ok;
    vm->topbar.maintenance_active = g_gs.maintenance_mode;
    vm->topbar.selftest_failed = !g_gs.selftest_passed;
    vm->topbar.carousel_paused = ui_screen_manager_carousel_is_paused();

    if (g_gs.time_valid) {
        time_t ts = 0;
        if (time_get(&ts) == ESP_OK) {
            struct tm tm_info;
            localtime_r(&ts, &tm_info);
            snprintf(vm->topbar.datetime_text, sizeof(vm->topbar.datetime_text),
                     "%02d/%02d/%04d %02d:%02d",
                     tm_info.tm_mday, tm_info.tm_mon + 1, tm_info.tm_year + 1900,
                     tm_info.tm_hour, tm_info.tm_min);
        }
    } else {
        snprintf(vm->topbar.datetime_text, sizeof(vm->topbar.datetime_text),
                 "Sem hora valida");
    }

    vm->footer.system_state = (ui_system_state_t)g_gs.system_state;
    vm->footer.active_alerts_count = g_gs.active_alerts_count;
    vm->footer.wizard_active = !g_gs.wizard_completed;
    vm->footer.time_valid = g_gs.time_valid;
    format_uptime(g_gs.uptime_s, vm->footer.uptime_text, sizeof(vm->footer.uptime_text));

    vm->dashboard.current_temp_c = g_gs.temp_filtered_c;
    const thermal_params_storage_t *tp = config_get_thermal();
    vm->dashboard.setpoint_temp_c = tp->temp_normal_c;
    vm->dashboard.thermal_state = map_thermal_from_gs();

    if (g_pzem.valid) {
        vm->dashboard.voltage_v = g_pzem.voltage_v;
        vm->dashboard.current_a = g_pzem.current_a;
        vm->dashboard.power_w = g_pzem.power_w;
        vm->dashboard.power_factor = g_pzem.pf;
    } else {
        vm->dashboard.voltage_v = 0.0f;
        vm->dashboard.current_a = 0.0f;
        vm->dashboard.power_w = 0.0f;
        vm->dashboard.power_factor = 0.0f;
    }

    vm->dashboard.ato_state = map_ato_from_gs();
    if (g_gs.ato_ok) {
        snprintf(vm->dashboard.ato_text, sizeof(vm->dashboard.ato_text), "ATO OK");
    } else {
        snprintf(vm->dashboard.ato_text, sizeof(vm->dashboard.ato_text), "ATO FALHA");
    }

    vm->energy.voltage_v = vm->dashboard.voltage_v;
    vm->energy.current_a = vm->dashboard.current_a;
    vm->energy.power_w = vm->dashboard.power_w;
    vm->energy.power_factor = vm->dashboard.power_factor;
    vm->energy.frequency_hz = g_pzem.valid ? g_pzem.frequency_hz : 0.0f;

    vm->devices.plug_count = UI_MAX_PLUGS;
    for (uint8_t i = 0; i < UI_MAX_PLUGS; i++) {
        plug_id_t pid = (plug_id_t)(i + 1);
        plug_model_t *plug = plug_manager_get(pid);
        ui_plug_vm_t *pv = &vm->devices.plugs[i];
        pv->plug_id = i + 1;
        snprintf(pv->code, sizeof(pv->code), "P%02u", (unsigned)(i + 1));
        if (plug) {
            snprintf(pv->name, sizeof(pv->name), "%s", plug->name);
            pv->state = map_plug_state(plug->effective_state);
            pv->current_a = plug->current_a;
            pv->current_valid = true;
            pv->is_critical = (pid == PLUG_ID_P01 || pid == PLUG_ID_P02);
        } else {
            pv->name[0] = '\0';
            pv->state = UI_PLUG_OFF;
            pv->current_a = 0.0f;
            pv->current_valid = false;
            pv->is_critical = (pid == PLUG_ID_P01 || pid == PLUG_ID_P02);
        }
    }

    alert_slot_t slots[UI_MAX_ALERTS];
    uint16_t slot_count = 0;
    alert_manager_get_active_slots(slots, &slot_count, UI_MAX_ALERTS);

    vm->alerts.active_count = 0;
    vm->alerts.critical_count = 0;
    vm->alerts.warning_count = 0;
    vm->alerts.high_count = 0;
    vm->alerts.info_count = 0;

    for (uint16_t i = 0; i < slot_count && vm->alerts.active_count < UI_MAX_ALERTS; i++) {
        if (!slots[i].active) {
            continue;
        }
        ui_alert_vm_t *av = &vm->alerts.active_alerts[vm->alerts.active_count++];
        snprintf(av->id, sizeof(av->id), "ALM-%03d", (int)slots[i].alm_id);
        av->severity = map_alert_severity(slots[i].severity);
        snprintf(av->severity_text, sizeof(av->severity_text), "%s",
                 slots[i].severity == ALERT_SEVERITY_CRITICAL ? "CRITICO" :
                 slots[i].severity == ALERT_SEVERITY_HIGH ? "ALTO" :
                 slots[i].severity == ALERT_SEVERITY_WARNING ? "AVISO" : "INFO");
        snprintf(av->message, sizeof(av->message), "%s", slots[i].message);
        snprintf(av->timestamp, sizeof(av->timestamp), "%llu",
                 (unsigned long long)slots[i].last_seen_ts);
        av->acked = slots[i].acked;
        snprintf(av->action_hint, sizeof(av->action_hint), "%s", slots[i].action_hint);

        switch (av->severity) {
            case UI_SEVERITY_CRITICAL: vm->alerts.critical_count++; break;
            case UI_SEVERITY_HIGH:     vm->alerts.high_count++; break;
            case UI_SEVERITY_WARNING:  vm->alerts.warning_count++; break;
            default:                   vm->alerts.info_count++; break;
        }
    }

    vm->diagnostics.temperature = map_health(health_get(SUB_SENSOR_TEMP));
    vm->diagnostics.ato = map_health(health_get(SUB_SENSOR_LEVEL));
    vm->diagnostics.energy = map_health(health_get(SUB_SENSOR_VOLTAGE));
    vm->diagnostics.plugs = map_health(health_get(SUB_RELAY_BOARD));
    vm->diagnostics.persistence = map_health(health_get(SUB_SD));
    vm->diagnostics.security = map_health(health_get(SUB_WIFI));
    vm->diagnostics.selftest = g_gs.selftest_passed ? UI_HEALTH_OK : UI_HEALTH_DEGRADED;
    vm->diagnostics.buses = map_health(health_get(SUB_BUS_SPI));
    vm->diagnostics.io = g_gs.ui_ok ? UI_HEALTH_OK : UI_HEALTH_FAILED;

    vm->config_temperature.setpoint_c = tp->temp_normal_c;
    vm->config_temperature.temp_min_c = tp->temp_min_c;
    vm->config_temperature.temp_max_c = tp->temp_max_c;
    vm->config_temperature.temp_critical_c = tp->temp_critical_c;
    vm->config_temperature.hysteresis_c = tp->hysteresis_c;
    vm->config_temperature.temp_extreme_c = tp->temp_extreme_c;

    vm->ato.state = map_ato_from_gs();
    vm->ato.level_adc = (g_gs.ato_ok) ? (int32_t)vm->dashboard.current_a : -1;
    {
        bool pump = false;
        ato_service_is_pump_on(&pump);
        vm->ato.pump_on = pump;
    }
    {
        bool ovf = false;
        ato_service_is_overflow(&ovf);
        vm->ato.overflow = ovf;
    }
}
