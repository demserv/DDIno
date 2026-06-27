#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "pin_map.h"
#include "global_state.h"
#include "hardware_config.h"
#include "hal_bus.h"
#include "hal_spi.h"
#include "driver_relay.h"
#include "driver_mcp3208.h"
#include "driver_acs712.h"
#include "driver_ad_keypad.h"
#include "driver_pzem.h"
#include "core/safety_controller.h"
#include "services/command_validator.h"
#include "services/alert_manager.h"
#include "services/safeoff_alm_map.h"
#include "services/electric_fsm.h"
#include "services/storage_sd.h"
#include "services/cdn_energy.h"
#include "services/config_manager.h"
#include "services/task_manager.h"
#include "services/wdt_advanced.h"
#include "fsm/thermal_fsm.h"
#include "drivers/driver_ds18b20.h"
#include "drivers/driver_ds3231.h"
#include "drivers/driver_buzzer_led.h"
#include "fsm/ato_fsm.h"
#include "web/api_rest.h"
#include "ui/ui_display.h"
#include "ui/ui_screens.h"
#include "ui/hmi/ui_app.h"
#include "core/circuit_breaker.h"
#include "services/self_test.h"
#include "services/temp_filter.h"
#include "services/audit_log.h"
#include "services/plug_manager.h"
#include "fsm/feed_fsm.h"
#include "fsm/restart_fsm.h"
#include "services/feed_snapshot.h"
#include "services/reset_handler.h"
#include "event_bus.h"
#include "health_matrix.h"
#include "wifi_ctl.h"
#include "sensor_ctl.h"
#include "disp_ctl.h"
#include "log_ctl.h"
#include "conf_ctl.h"
#include "web_ctl.h"
#include "alm_ctl.h"
#include "bom_ctl.h"
#include "data_ctl.h"
#include "thermal_service.h"
#include "ato_service.h"
#include "electric_service.h"
#include "time_manager.h"
#include "pending_actions.h"
#include "sec_policy.h"
#include "watchdog_guard.h"

static const char *TAG = "app_main";

global_state_t g_gs;
pzem_data_t g_pzem;
bool g_feed_request = false;
restart_fsm_t g_restart_fsm;

static thermal_fsm_t s_thermal_fsm;
static ato_fsm_t s_ato_fsm;
static electric_fsm_t s_electric_fsm;
static feed_fsm_t s_feed_fsm;
static thermal_params_t s_tcfg;
static electric_params_t s_ecfg;
static ato_params_t s_acfg;

static esp_err_t init_nvs_safe(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS inconsistente, executando recovery");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void init_global_state(void)
{
    memset(&g_gs, 0, sizeof(g_gs));
    g_gs.system_state = SYSTEM_STATE_NORMAL;
    g_gs.safeoff_reason = SAFEOFF_REASON_NONE;
    g_gs.last_reset_reason = (uint32_t)esp_reset_reason();
    snprintf(g_gs.fw_version, sizeof(g_gs.fw_version), "F1.0.0");
    snprintf(g_gs.srs_version, sizeof(g_gs.srs_version), "v3.11-AF.3+AF.4+B12/N+B13/N");
    snprintf(g_gs.config_schema_version, sizeof(g_gs.config_schema_version), "1.0");

    const system_params_storage_t *sys = config_get_system();
    g_gs.wizard_completed = sys->wizard_completed;
    g_gs.monitor_only_mode = sys->monitor_only_mode;
    g_gs.feed_active = false;
    g_gs.feed_remaining_s = 0;
    g_gs.feed_state = FEED_STATE_IDLE;
    g_gs.health_check_interval_s = 60;
    g_gs.temp_filtered_c = 0.0f;
    g_gs.restart_in_progress = false;
    g_gs.wizard_step = (wizard_step_t)config_get_wizard_step();
}

static bool read_temp(float *temp_c)
{
    if (!temp_c) return false;
    if (!circuit_breaker_is_available(CB_BUS_DS18B20)) return false;
    if (ds18b20_read(temp_c)) {
        circuit_breaker_record_success(CB_BUS_DS18B20);
        return true;
    }
    circuit_breaker_record_failure(CB_BUS_DS18B20);
    return false;
}

static bool read_ato_level(int32_t *level_adc)
{
    if (!level_adc) return false;
    if (!circuit_breaker_is_available(CB_BUS_SPI_ADC)) return false;
    uint16_t adc = 0;
    esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &adc);
    if (err != ESP_OK) {
        circuit_breaker_record_failure(CB_BUS_SPI_ADC);
        return false;
    }
    circuit_breaker_record_success(CB_BUS_SPI_ADC);
    *level_adc = (int32_t)adc;
    return true;
}

static bool safeoff_cause_is_resolved(void)
{
    switch (g_gs.safeoff_reason) {
        case SAFEOFF_REASON_THERMAL_CRITICAL:
        case SAFEOFF_REASON_THERMAL_EXTREME: {
            if (!g_gs.temp_ok) return false;
            return g_gs.temp_filtered_c < (float)s_tcfg.temp_critical_c;
        }
        case SAFEOFF_REASON_ATO_OVERFLOW: {
            if (!g_gs.ato_ok) return false;
            int32_t level_adc = 0;
            if (!read_ato_level(&level_adc)) return false;
            int32_t overflow = (int32_t)s_acfg.high_level_adc + (int32_t)s_acfg.overflow_margin_adc;
            return level_adc <= overflow;
        }
        case SAFEOFF_REASON_ELECTRIC_TOTAL:
        case SAFEOFF_REASON_OVERVOLTAGE:
        case SAFEOFF_REASON_UNDERVOLTAGE: {
            if (!g_gs.pzem_ok) return false;
            return g_pzem.power_w <= (float)s_ecfg.total_power_limit_w;
        }
        case SAFEOFF_REASON_PLUG_SHORT: {
            if (!g_gs.pzem_ok) return false;
            return g_pzem.current_a <= (float)s_ecfg.per_plug_current_limit_a;
        }
        case SAFEOFF_REASON_SELFTEST_FAIL:
            return g_gs.selftest_passed;
        default:
            return true;
    }
}

static void handle_safeoff_exit(uint64_t now_s, uint64_t now_ms)
{
    safety_inputs_t sin;
    memset(&sin, 0, sizeof(sin));
    sin.safeoff_cause_resolved = safeoff_cause_is_resolved();
    sin.all_sensors_valid = (g_gs.temp_ok && g_gs.pzem_ok);
    sin.selftest_passed = g_gs.selftest_passed;
    sin.manual_ack_received = true;
    sin.cause_resolved_at_ms = (now_s - 5) * 1000ULL;

    if (g_gs.restart_in_progress && restart_fsm_is_complete(&g_restart_fsm)) {
        ESP_LOGW(TAG, "RESTART: sequencia completa -> NORMAL");
        g_gs.system_state = SYSTEM_STATE_NORMAL;
        g_gs.safeoff_reason = SAFEOFF_REASON_NONE;
        g_gs.restart_in_progress = false;
        g_gs.safeoff_source_alm[0] = '\0';
        plug_manager_set_restart_mask(0);
        audit_log_event(AUDIT_SAFE_OFF, "Restart sequence complete -> NORMAL");
        restart_fsm_abort(&g_restart_fsm);
        return;
    }

    if (g_gs.restart_in_progress && (!sin.safeoff_cause_resolved || !sin.all_sensors_valid)) {
        ESP_LOGW(TAG, "RESTART: condicoes perdidas, abortando religamento");
        g_gs.restart_in_progress = false;
        plug_manager_set_restart_mask(0);
        relay_all_off();
        audit_log_event(AUDIT_SAFE_OFF, "Restart aborted - conditions lost");
        restart_fsm_abort(&g_restart_fsm);
        return;
    }

    if (g_gs.restart_in_progress) {
        restart_fsm_update(&g_restart_fsm, now_ms);
        plug_manager_set_restart_mask(restart_fsm_energized_mask(&g_restart_fsm));
        return;
    }

    if (safety_controller_can_exit_safeoff(&g_gs, &sin, now_s)) {
        ESP_LOGW(TAG, "RESTART: condicoes de saida SAFE_OFF atendidas, religando");
        g_gs.restart_in_progress = true;
        audit_log_event(AUDIT_SAFE_OFF, "Restart sequence started");
        restart_fsm_start(&g_restart_fsm, now_ms);
    }
}

static void handle_emergency_exit(uint64_t now_s)
{
    safety_inputs_t sin;
    memset(&sin, 0, sizeof(sin));
    sin.emergency_resolved = true;
    sin.all_sensors_valid = (g_gs.temp_ok && g_gs.pzem_ok);
    sin.manual_ack_received = true;
    sin.cause_resolved_at_ms = (now_s - 5) * 1000ULL;

    if (safety_controller_can_exit_emergency(&g_gs, &sin, now_s)) {
        ESP_LOGW(TAG, "EMERGENCY exit conditions met -> SAFE_OFF");
        global_state_enter_safeoff(&g_gs, SAFEOFF_REASON_FSM_INVALID, "ALM-003", "emergency_exit", now_s);
    }
}

static void reset_handler_check(uint64_t now_ms)
{
    static bool s_was_up_held = false;
    static uint64_t s_up_held_start_ms = 0;

    reset_state_t rst = reset_handler_get_state();
    uint16_t adc_val = 4096;
    mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc_val);
    bool up_pressed = (adc_val >= 100 && adc_val < 600);

    if (rst == RESET_STATE_CONFIRM1) {
        if (up_pressed && !s_was_up_held) {
            reset_handler_confirm();
            s_was_up_held = true;
        } else if (!up_pressed) {
            s_was_up_held = false;
        }
    } else if (rst == RESET_STATE_IDLE && up_pressed) {
        if (!s_was_up_held) {
            s_up_held_start_ms = now_ms;
            s_was_up_held = true;
        } else if (now_ms - s_up_held_start_ms >= 10000) {
            reset_handler_start();
            s_was_up_held = false;
            s_up_held_start_ms = 0;
        }
    } else if (!up_pressed) {
        s_was_up_held = false;
        s_up_held_start_ms = 0;
    }

    int rem = reset_handler_remaining_s();
    switch (rst) {
        case RESET_STATE_CONFIRM1:
            snprintf(g_gs.reset_status_msg, sizeof(g_gs.reset_status_msg),
                     "FACTORY RESET: Solte UP e pressione novamente para confirmar");
            break;
        case RESET_STATE_CONFIRM2:
            snprintf(g_gs.reset_status_msg, sizeof(g_gs.reset_status_msg),
                     "Confirmado! Reset automatico em %ds...", rem);
            break;
        case RESET_STATE_COUNTDOWN:
            snprintf(g_gs.reset_status_msg, sizeof(g_gs.reset_status_msg),
                     "FACTORY RESET em %ds", rem);
            break;
        case RESET_STATE_ERASING:
            snprintf(g_gs.reset_status_msg, sizeof(g_gs.reset_status_msg),
                     "APAGANDO NVS...");
            break;
        default:
            g_gs.reset_status_msg[0] = '\0';
            break;
    }
}

static void read_sensors(float *temp_c, int32_t *ato_level)
{
    *temp_c = 25.0f;
    *ato_level = 0;

    bool temp_valid = read_temp(temp_c);
    static bool s_temp_was_valid_local = true;
    if (temp_valid && !s_temp_was_valid_local) {
        alert_manager_clear(ALM_013);
    }
    s_temp_was_valid_local = temp_valid;
    g_gs.temp_ok = temp_valid;

    if (temp_valid) {
        g_gs.temp_filtered_c = temp_filter_update(*temp_c);
    }

    bool ato_valid = read_ato_level(ato_level);
    g_gs.ato_ok = ato_valid;
}

static void update_plug_currents(float *plug_currents)
{
    if (!circuit_breaker_is_available(CB_BUS_SPI_ADC)) return;
    for (uint8_t plug = 1; plug <= 10; plug++) {
        float cur = 0;
        acs712_read_plug(plug, &cur);
        plug_currents[plug - 1] = cur;
    }
}

static void read_pzem(void)
{
    if (!circuit_breaker_is_available(CB_BUS_UART_PZEM)) return;
    memset(&g_pzem, 0, sizeof(g_pzem));
    esp_err_t perr = pzem_read_all(&g_pzem);
    if (perr != ESP_OK) {
        circuit_breaker_record_failure(CB_BUS_UART_PZEM);
    } else {
        circuit_breaker_record_success(CB_BUS_UART_PZEM);
    }
    g_gs.pzem_ok = g_pzem.valid;
}

static void update_energy_accumulators(const float *plug_currents)
{
    double total_wh = 0;
    float voltage = (g_pzem.valid && g_pzem.voltage_v > 0.0f) ? g_pzem.voltage_v : 127.0f;
    for (uint8_t p = 1; p <= 10; p++) {
        cdn_energy_update(p, plug_currents[p-1], voltage, 50);
        total_wh += cdn_energy_get_wh(p);
    }
    g_pzem.energy_wh = (float)total_wh;
}

static void update_feed_snapshot(uint64_t now_ms, uint64_t now_s)
{
    static uint64_t s_last_feed_snap_ms = 0;
    feed_state_t fst = feed_fsm_get_state(&s_feed_fsm);
    if ((fst == FEED_STATE_ACTIVE || fst == FEED_STATE_COOLDOWN) &&
        (now_ms - s_last_feed_snap_ms > 5000)) {
        feed_snapshot_save(&s_feed_fsm, now_s, g_gs.time_valid);
        s_last_feed_snap_ms = now_ms;
    } else if (fst == FEED_STATE_IDLE && s_last_feed_snap_ms != 0) {
        feed_snapshot_clear();
        s_last_feed_snap_ms = 0;
    }

    if (g_feed_request && g_gs.system_state == SYSTEM_STATE_NORMAL) {
        if (feed_fsm_start(&s_feed_fsm, now_ms)) {
            audit_log_event(AUDIT_FEED_MODE, "Feed mode started");
        }
        g_feed_request = false;
    }

    g_gs.feed_active = (feed_fsm_get_state(&s_feed_fsm) == FEED_STATE_ACTIVE);
    g_gs.feed_state = feed_fsm_get_state(&s_feed_fsm);
    g_gs.feed_remaining_s = feed_fsm_remaining_s(&s_feed_fsm);
}

static void update_led_signaling(uint64_t now_ms)
{
    static uint64_t s_last_blink_ms = 0;
    static bool s_blink_on = false;

    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        if (now_ms - s_last_blink_ms > 500) {
            s_blink_on = !s_blink_on;
            s_last_blink_ms = now_ms;
            if (s_blink_on) led_all_on();
            else led_all_off();
        }
    } else if (g_gs.system_state == SYSTEM_STATE_DEGRADED) {
        led_set(LED_YELLOW);
    } else {
        led_set(LED_GREEN);
    }
}

static void log_energy_if_due(uint64_t now_s)
{
    static uint64_t s_last_log_s = 0;
    if (now_s - s_last_log_s >= HW_SD_LOG_INTERVAL_S) {
        cdn_energy_log_to_sd();
        s_last_log_s = now_s;
    }
}

static void check_hw_alert(uint64_t now_s)
{
    const thermal_output_t *tout = thermal_fsm_get_output(&s_thermal_fsm);
    bool thermal_sensor_fault = (tout && tout->sensor_fault);

    if (thermal_sensor_fault && !g_gs.hw_alert_pending
        && g_gs.system_state < SYSTEM_STATE_SAFE_OFF
        && g_gs.system_state != SYSTEM_STATE_EMERGENCY) {
        const alert_slot_t *slot = alert_manager_get_slot(ALM_013);
        if (!slot) {
            g_gs.hw_alert_pending = true;
            g_gs.hw_alert_alm_id = ALM_013;
            snprintf(g_gs.hw_alert_msg, sizeof(g_gs.hw_alert_msg),
                     "Sensor de temperatura sem comunicacao. Toque para continuar.");
            alert_manager_raise_full(ALM_013, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_PROCESS,
                                     g_gs.hw_alert_msg, 0.0f,
                                     "Verificar sensor DS18B20",
                                     0, true, false, now_s);
            audit_log_event(AUDIT_SAFE_OFF, "Temp sensor fail -> alerta critico (hw_alert)");
        }
    }
}

static void update_safety_outputs(const float *plug_currents, const thermal_output_t *tout,
                                   const ato_output_t *aout, const electric_output_t *eout,
                                   uint64_t now_s, uint64_t now_ms, system_state_t *prev_state)
{
    bool thermal_sensor_fault = (tout && tout->sensor_fault);

    safety_inputs_t sin;
    memset(&sin, 0, sizeof(sin));
    sin.emergency_condition = (tout && tout->force_emergency);
    sin.safeoff_condition = ((tout && tout->force_safe_off && !thermal_sensor_fault)
                         || (aout && aout->force_safe_off)
                         || (eout && eout->force_safe_off));
    sin.degraded_condition = false;
    sin.all_sensors_valid = (g_gs.temp_ok && g_gs.pzem_ok);
    sin.selftest_passed = g_gs.selftest_passed;
    sin.emergency_resolved = !sin.emergency_condition;
    sin.safeoff_cause_resolved = !sin.safeoff_condition;

    if (tout && tout->force_safe_off && !thermal_sensor_fault) {
        sin.safeoff_reason_if_any = tout->safeoff_reason;
    } else if (aout && aout->force_safe_off) {
        sin.safeoff_reason_if_any = aout->safeoff_reason;
    } else if (eout && eout->force_safe_off) {
        sin.safeoff_reason_if_any = eout->safeoff_reason;
    } else {
        sin.safeoff_reason_if_any = SAFEOFF_REASON_NONE;
    }
    sin.transition_cause = "runtime_tick";

    if (tout && tout->force_emergency) sin.safeoff_source_alm = "ALM-028";
    else if (tout && tout->force_safe_off && !thermal_sensor_fault) sin.safeoff_source_alm = "ALM-026";
    else if (aout && aout->force_safe_off) sin.safeoff_source_alm = "ALM-037";
    else if (eout && eout->force_safe_off) {
        sin.safeoff_source_alm = (eout->suggested_alm == ALM_052) ? "ALM-052" : "ALM-055";
    }

    safety_controller_evaluate(&g_gs, &sin, now_s);

    if (g_gs.system_state != *prev_state) {
        static const char *state_names[] = {"NORMAL","DEGRADED","SAFE_OFF","EMERGENCY"};
        const char *from_s = (*prev_state < 4) ? state_names[*prev_state] : "?";
        const char *to_s = (g_gs.system_state < 4) ? state_names[g_gs.system_state] : "?";
        audit_log_state_change(from_s, to_s, sin.transition_cause);
        *prev_state = g_gs.system_state;
    }

    if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF && g_gs.safeoff_reason != SAFEOFF_REASON_NONE) {
        int16_t alm = safeoff_reason_to_alm_id(g_gs.safeoff_reason);
        if (alm > 0) {
            alert_manager_raise((uint16_t)alm, true, now_s);
        }
    }
}

static void task_safety_core_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "safety_core task iniciada");

    system_state_t prev_state = g_gs.system_state;

    static uint32_t s_heartbeat_check_cycle = 0;

    while (1) {
        wdt_advanced_reset(TASK_ID_SAFETY_CORE);
        watchdog_guard_heartbeat(TASK_ID_SAFETY_CORE);

        circuit_breaker_update();
        event_bus_process_pending();
        health_matrix_update();

        s_heartbeat_check_cycle++;
        if (s_heartbeat_check_cycle % 20 == 0) {
            for (int i = 0; i < TASK_ID_COUNT; i++) {
                if (i == TASK_ID_SAFETY_CORE) continue;
                if (i == TASK_ID_DIAG && !watchdog_guard_get_heartbeat(i)) continue;
                watchdog_status_t st = watchdog_guard_check(i);
                if (st.alarmed) {
                    if (st.severity == WATCHDOG_SEVERITY_CRITICAL) {
                        ESP_LOGE(TAG, "Task %d critica sem heartbeat -> SAFE_OFF", i);
                        global_state_enter_safeoff(&g_gs, SAFEOFF_REASON_WDT_RECOVERY,
                                                   "ALM-043", "task_hang", now_s);
                    } else {
                        ESP_LOGW(TAG, "Task %d sem heartbeat -> alerta ALM-043", i);
                        const char *tn = task_manager_get_name(i);
                        alert_manager_raise_full(ALM_043, ALERT_SEVERITY_HIGH,
                                                 ALERT_CATEGORY_SYSTEM,
                                                 "Task sem heartbeat", 0.0f,
                                                 tn ? tn : "unknown",
                                                 0, false, true, now_s);
                    }
                }
                if (st.recovered && g_gs.system_state != SYSTEM_STATE_SAFE_OFF) {
                    alert_manager_clear(ALM_043);
                }
            }
        }

        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
        const uint64_t now_s = now_ms / 1000ULL;

        reset_handler_tick(now_ms);
        reset_handler_check(now_ms);

        if (reset_handler_is_pending()) {
            wdt_advanced_reset(TASK_ID_SAFETY_CORE);
            feed_fsm_update(&s_feed_fsm, now_ms);
            lv_timer_handler();
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (g_gs.system_state == SYSTEM_STATE_EMERGENCY) {
            handle_emergency_exit(now_s);
        }
        if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF) {
            handle_safeoff_exit(now_s, now_ms);
        }

        float temp_c = 25.0f;
        int32_t ato_level = 0;
        float plug_currents[10] = {0};

        read_sensors(&temp_c, &ato_level);

        thermal_input_t tin = {
            .sample_valid = g_gs.temp_ok,
            .temp_c = g_gs.temp_filtered_c,
            .now_ms = now_ms
        };
        ato_input_t ain = {
            .sample_valid = g_gs.ato_ok,
            .level_adc = ato_level,
            .now_ms = now_ms
        };

        if (!g_gs.wizard_completed) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        thermal_fsm_update(&s_thermal_fsm, &tin);
        ato_fsm_update(&s_ato_fsm, &ain);

        update_plug_currents(plug_currents);
        read_pzem();

        electric_input_t ein;
        memset(&ein, 0, sizeof(ein));
        ein.sample_valid = g_pzem.valid;
        ein.total_power_w = g_pzem.power_w;
        ein.total_energy_wh = g_pzem.energy_wh;
        ein.plug_count = 10;
        ein.voltage_v = g_pzem.voltage_v;
        ein.frequency_hz = g_pzem.frequency_hz;
        ein.pf = g_pzem.pf;
        ein.now_ms = now_ms;
        memcpy(ein.plug_currents_a, plug_currents, sizeof(ein.plug_currents_a));
        electric_fsm_update(&s_electric_fsm, &ein);

        const thermal_output_t *tout = thermal_fsm_get_output(&s_thermal_fsm);
        const ato_output_t *aout = ato_fsm_get_output(&s_ato_fsm);
        const electric_output_t *eout = electric_fsm_get_output(&s_electric_fsm);

        update_safety_outputs(plug_currents, tout, aout, eout, now_s, now_ms, &prev_state);
        check_hw_alert(now_s);

        g_gs.active_alerts_count = alert_manager_active_count();
        g_gs.critical_alerts_count = alert_manager_critical_count();
        g_gs.uptime_s = now_s;

        update_energy_accumulators(plug_currents);
        feed_fsm_update(&s_feed_fsm, now_ms);
        update_feed_snapshot(now_ms, now_s);

        plug_manager_set_thermal_request(PLUG_ID_P02,
            tout ? tout->request_heater_on : false,
            tout ? tout->request_cooler_on : false);
        plug_manager_tick(now_s, g_gs.system_state, g_gs.feed_active);

        if (now_s % g_gs.health_check_interval_s == 0) {
            g_gs.last_health_check_timestamp = now_s;
            g_gs.sd_ok = storage_sd_is_mounted();
        }

        log_energy_if_due(now_s);
        update_led_signaling(now_ms);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void task_sensors_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "sensors task iniciada");

    while (1) {
        wdt_advanced_reset(TASK_ID_SENSORS);
        watchdog_guard_heartbeat(TASK_ID_SENSORS);
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_SENSORS));
    }
}

static void task_plug_control_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "plug_control task iniciada");

    while (1) {
        wdt_advanced_reset(TASK_ID_PLUG_CONTROL);
        watchdog_guard_heartbeat(TASK_ID_PLUG_CONTROL);
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_PLUG_CONTROL));
    }
}

static void task_storage_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "storage task iniciada");

    while (1) {
        wdt_advanced_reset(TASK_ID_STORAGE);
        watchdog_guard_heartbeat(TASK_ID_STORAGE);
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_STORAGE));
    }
}

static void task_ui_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "ui task iniciada");

    while (1) {
        wdt_advanced_reset(TASK_ID_UI);
        watchdog_guard_heartbeat(TASK_ID_UI);
        ui_screen_update_all();
        ui_app_tick();
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_UI));
    }
}

static void task_web_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "web task iniciada");

    while (1) {
        wdt_advanced_reset(TASK_ID_WEB);
        watchdog_guard_heartbeat(TASK_ID_WEB);
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_WEB));
    }
}

static void task_diag_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "diag task iniciada");

    while (1) {
        wdt_advanced_reset(TASK_ID_DIAG);
        watchdog_guard_heartbeat(TASK_ID_DIAG);
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_DIAG));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Monitor Aquario Inteligente - Boot");
    ESP_LOGI(TAG, "SRS v3.11-AF.3+AF.4+B12/N+B13/N");
    ESP_LOGI(TAG, "========================================");

    ESP_ERROR_CHECK(init_nvs_safe());
    ESP_LOGI(TAG, "Reset reason: %d", (int)esp_reset_reason());

    ESP_ERROR_CHECK(config_manager_init());
    init_global_state();
    safety_controller_init(&g_gs);
    {
        const restart_params_storage_t *rp = config_get_restart();
        restart_cfg_t rcfg = {
            .wait_time_ms = rp->tempo_espera_religamento_s * 1000U,
            .stagger_interval_ms = rp->intervalo_religamento_s * 1000U,
            .monitor_time_ms = rp->tempo_monitoramento_pos_relig_s * 1000U
        };
        restart_fsm_init(&g_restart_fsm, &rcfg);
    }
    alert_manager_init();
    self_test_init();

    ESP_ERROR_CHECK(hal_bus_init_all());
    ESP_ERROR_CHECK(hal_spi_init());
    circuit_breaker_init();
    esp_err_t relay_err = relay_init_safe();
    if (relay_err != ESP_OK) {
        ESP_LOGE(TAG, "MODULO DE RELE NAO DETECTADO! Reles P03-P10 inoperantes");
        g_gs.hw_ok = false;
    } else {
        ESP_LOGI(TAG, "Reles em OFF seguro (P01..P10)");
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(mcp3208_init(PIN_ADC1_CS_GPIO));
    acs712_init();
    {
        const calibration_params_storage_t *cp = config_get_calibration();
        for (uint8_t p = 1; p <= 10; p++) {
            acs712_set_zero_offset(p, cp->acs712_zero_offset_mv[p - 1]);
        }
        ESP_LOGI(TAG, "Calibracao: offsets ACS712 aplicados da NVS");
    }
    ad_keypad_init(NULL);
    esp_err_t pzem_err = pzem_init();
    if (pzem_err != ESP_OK) {
        ESP_LOGW(TAG, "PZEM init falhou: %s", esp_err_to_name(pzem_err));
    }
    ds18b20_init();
    ds3231_init();
    buzzer_led_init();
    cdn_energy_init();

    esp_err_t sd_err = storage_sd_init();
    if (sd_err != ESP_OK) {
        ESP_LOGW(TAG, "SD nao disponivel (sistema continua sem storage)");
    }

    {
        const thermal_params_storage_t *tp = config_get_thermal();
        const electric_params_storage_t *ep = config_get_electric();
        const ato_params_storage_t *ap = config_get_ato();
        s_ecfg = (electric_params_t){
            .total_power_limit_w = ep->total_power_limit_w,
            .per_plug_current_limit_a = ep->per_plug_current_limit_a,
            .hysteresis_w = ep->hysteresis_w,
            .overvoltage_limit_v = ep->overvoltage_limit_v,
            .undervoltage_limit_v = ep->undervoltage_limit_v,
            .overvoltage_time_s = ep->overvoltage_time_s,
            .undervoltage_time_s = ep->undervoltage_time_s,
            .total_current_limit_a = ep->total_current_limit_a,
            .total_current_time_s = ep->total_current_time_s,
            .pf_min = ep->pf_min,
            .pf_time_s = ep->pf_time_s,
            .fator_curto = ep->fator_curto,
            .tempo_deteccao_curto_ms = ep->tempo_deteccao_curto_ms
        };
        s_tcfg = (thermal_params_t){
            .temp_normal_c = tp->temp_normal_c,
            .temp_critical_c = tp->temp_critical_c,
            .temp_extreme_c = tp->temp_extreme_c,
            .hysteresis_c = tp->hysteresis_c,
            .extreme_enabled = tp->extreme_enabled
        };
        s_acfg = (ato_params_t){
            .enabled = ap->enabled,
            .low_level_adc = ap->low_level_adc,
            .high_level_adc = ap->high_level_adc,
            .overflow_margin_adc = ap->overflow_margin_adc,
            .refill_timeout_s = ap->refill_timeout_s
        };
    }

    thermal_fsm_init(&s_thermal_fsm, &s_tcfg);
    ato_fsm_init(&s_ato_fsm, &s_acfg);
    electric_fsm_init(&s_electric_fsm, &s_ecfg);
    temp_filter_init(5);
    plug_manager_init();
    const feed_params_storage_t *fp = config_get_feed();
    feed_fsm_init(&s_feed_fsm, fp->feed_duration_min * 60, 120);
    {
        uint64_t boot_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
        esp_err_t snap_err = feed_snapshot_restore(&s_feed_fsm, 0, boot_ms, g_gs.time_valid);
        if (snap_err == ESP_OK && s_feed_fsm.state != FEED_STATE_IDLE) {
            ESP_LOGW(TAG, "Feed snapshot restored (state=%d)", (int)s_feed_fsm.state);
            audit_log_event(AUDIT_FEED_MODE, "Feed state restored from NVS snapshot");
        }
    }

    esp_err_t ui_err = ui_display_init();
    if (ui_err != ESP_OK) {
        ESP_LOGW(TAG, "Display LVGL nao disponivel (sistema continua sem UI)");
    }
    ui_screens_init();
    ui_app_init();

    esp_netif_init();
    esp_err_t api_err = api_rest_init();
    if (api_err != ESP_OK) {
        ESP_LOGW(TAG, "API web nao iniciou (sistema continua sem API)");
    }

    event_bus_init();
    health_matrix_init();

    conf_ctl_init();
    log_ctl_init();
    disp_ctl_init();
    sensor_ctl_init();
    wifi_ctl_init();
    web_ctl_init();
    alm_ctl_init();
    bom_ctl_init();
    data_ctl_init();

    thermal_service_init();
    ato_service_init();
    electric_service_init();
    time_manager_init();
    pending_actions_init();
    sec_policy_init();

    ESP_LOGI(TAG, "Init completo - lancando tasks normativas");

    wdt_advanced_init();
    watchdog_guard_init();

    {
        const selftest_params_storage_t *sp = config_get_selftest();
        esp_err_t st = self_test_run_all(sp->selftest_timeout_ms);
        if (st != ESP_OK) {
            ESP_LOGW(TAG, "Self-test incompleto ou timeout: %s", esp_err_to_name(st));
        }
        self_test_log_results();
        g_gs.selftest_passed = self_test_all_passed();
        g_gs.hw_ok = self_test_critical_passed();
    }

    if (!g_gs.selftest_passed || !g_gs.hw_ok) {
        ESP_LOGE(TAG, "SELF-TEST FALHOU! Hardware com problemas -> DEGRADED + alerta");
        uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
        global_state_enter_degraded(&g_gs, "selftest_fail");
        g_gs.hw_alert_pending = true;
        g_gs.hw_alert_alm_id = ALM_063;
        snprintf(g_gs.hw_alert_msg, sizeof(g_gs.hw_alert_msg),
                 "Falha de hardware na inicializacao. Toque para continuar em modo degradado.");
        alert_manager_raise_full(ALM_063, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_SYSTEM,
                                 g_gs.hw_alert_msg, 0.0f,
                                 "Hardware nao detectado. Sistema operara sem protecoes.",
                                 0, true, false, now_s);
        audit_log_event(AUDIT_SELF_TEST, "Self-test falhou -> DEGRADED (hw alert)");
    }

    reset_handler_init();

    ESP_LOGI(TAG, "FASE 7 - Lancando tasks via task_manager");

    task_manager_launch_all();
    task_manager_create(task_safety_core_fn, TASK_ID_SAFETY_CORE);
    task_manager_create(task_sensors_fn, TASK_ID_SENSORS);
    task_manager_create(task_plug_control_fn, TASK_ID_PLUG_CONTROL);
    task_manager_create(task_storage_fn, TASK_ID_STORAGE);
    task_manager_create(task_ui_fn, TASK_ID_UI);
    task_manager_create(task_web_fn, TASK_ID_WEB);
    task_manager_create(task_diag_fn, TASK_ID_DIAG);

    vTaskDelete(NULL);
}
