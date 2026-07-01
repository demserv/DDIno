// @requirement RF-FLOW-BOOT-001 a RF-FLOW-BOOT-010 RF-FLOW-RUNTIME-001
// @requirement RF-FLOW-BOOT-001 Ponto de entrada principal do firmware
// @requirement RF-FLOW-BOOT-002 Inicializacao sequencial de modulos
// @requirement RF-FLOW-BOOT-003 Self-test no boot com fallback SAFE_OFF
// @requirement RF-FLOW-BOOT-004 Restauracao de estado apos reset
// @requirement RF-MAINTENANCE-001 Modo de manutencao via wizard
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
#include "command_validator.h"
#include "alert_manager.h"
#include "safeoff_alm_map.h"
#include "electric_fsm.h"
#include "storage_sd.h"
#include "safeoff_record.h"
#include "safe_state_ack.h"
#include "health_matrix.h"
#include "auth_recovery_sd.h"
#include "wdt_stats.h"
#include "cdn_energy.h"
#include "electric_log.h"
#include "profile_manager.h"
#include "config_manager.h"
#include "task_manager.h"
#include "wdt_advanced.h"
#include "fsm/thermal_fsm.h"
#include "drivers/driver_ds18b20.h"
#include "drivers/driver_ph_sensor.h"
#include "drivers/driver_ds3231.h"
#include "drivers/driver_buzzer_led.h"
#include "fsm/ato_fsm.h"
#include "web/api_rest.h"
#include "web/api_auth.h"
#include "drivers/driver_ili9488.h"
#include "drivers/driver_xpt2046.h"
#include "drivers/driver_ad_keypad_lvgl.h"
#include "ui/hmi/ui_app.h"
#include "ui/hmi/ui_screen_manager.h"
#include "ui/hmi/ui_lvgl_mutex.h"
#include "core/circuit_breaker.h"
#include "self_test.h"
#include "temp_filter.h"
#include "audit_log.h"
#include "plug_manager.h"
#include "fsm/feed_fsm.h"
#include "fsm/restart_fsm.h"
#include "feed_snapshot.h"
#include "reset_handler.h"
#include "event_bus.h"
#include "health_matrix.h"
#include "wifi_ctl.h"
#include "sensor_ctl.h"
#include "disp_ctl.h"
#include "log_ctl.h"
#include "storage_facade.h"
#include "thermal_service.h"
#include "ato_service.h"
#include "electric_service.h"
#include "time_manager.h"
#include "pending_actions.h"
#include "sec_policy.h"
#include "watchdog_guard.h"
#include "alm_monitor.h"
#include "maintenance_mode.h"

extern void task_ui_fn(void *pv);
extern void task_web_fn(void *pv);

static const char *TAG = "app_main";

global_state_t g_gs;
pzem_data_t g_pzem;
bool g_feed_request = false;
bool g_feed_exit_request = false;
bool g_feed_ui_pending = false;
restart_fsm_t g_restart_fsm;

static thermal_fsm_t s_thermal_fsm;
static ato_fsm_t s_ato_fsm;
static electric_fsm_t s_electric_fsm;
static feed_fsm_t s_feed_fsm;
static thermal_params_t s_tcfg;
static electric_params_t s_ecfg;
static ato_params_t s_acfg;

static uint64_t s_safeoff_resolved_at_ms;
static uint64_t s_emergency_resolved_at_ms;
static int16_t s_last_tout_alm = 0;
static int16_t s_last_aout_alm = 0;
static int16_t s_last_eout_alm = 0;

static void fsm_alm_sync(int16_t cur, int16_t *last, bool allow_raise, uint64_t now_s)
{
    if (*last != 0 && (cur == 0 || !allow_raise || *last != cur)) {
        alert_manager_try_auto_clear(*last, true);
        *last = 0;
    }
    if (cur != 0 && allow_raise) {
        if (*last != cur) {
            alert_manager_raise(cur, true, now_s);
            *last = cur;
        }
    }
}

void app_ato_clear_blocked(void)
{
    if (!maintenance_mode_is_active()) return;
    ato_fsm_clear_blocked(&s_ato_fsm);
}

static void app_reload_fsms_from_config(void)
{
    const thermal_params_storage_t *tp = config_get_thermal();
    const ato_params_storage_t *ap = config_get_ato();
    const electric_params_storage_t *ep = config_get_electric();
    const feed_params_storage_t *fp = config_get_feed();
    const restart_params_storage_t *rp = config_get_restart();
    if (tp) {
        s_tcfg.temp_normal_c = tp->temp_normal_c;
        s_tcfg.temp_critical_c = tp->temp_critical_c;
        s_tcfg.temp_extreme_c = tp->temp_extreme_c;
        s_tcfg.temp_min_c = tp->temp_min_c;
        s_tcfg.temp_max_c = tp->temp_max_c;
        s_tcfg.hysteresis_c = tp->hysteresis_c;
        s_tcfg.extreme_enabled = tp->extreme_enabled;
        thermal_fsm_set_config(&s_thermal_fsm, &s_tcfg);
    }
    if (ap) {
        s_acfg.enabled = ap->enabled;
        s_acfg.low_level_adc = ap->low_level_adc;
        s_acfg.high_level_adc = ap->high_level_adc;
        s_acfg.overflow_margin_adc = ap->overflow_margin_adc;
        s_acfg.refill_timeout_s = ap->refill_timeout_s;
        ato_fsm_set_config(&s_ato_fsm, &s_acfg);
    }
    if (ep) {
        s_ecfg.overvoltage_limit_v = ep->overvoltage_limit_v;
        s_ecfg.undervoltage_limit_v = ep->undervoltage_limit_v;
        s_ecfg.overvoltage_time_s = ep->overvoltage_time_s;
        s_ecfg.undervoltage_time_s = ep->undervoltage_time_s;
        s_ecfg.total_power_limit_w = ep->total_power_limit_w;
        s_ecfg.total_current_limit_a = ep->total_current_limit_a;
        s_ecfg.total_current_time_s = ep->total_current_time_s;
        s_ecfg.per_plug_current_limit_a = ep->per_plug_current_limit_a;
        s_ecfg.pf_min = ep->pf_min;
        s_ecfg.pf_time_s = ep->pf_time_s;
        s_ecfg.hysteresis_w = ep->hysteresis_w;
        s_ecfg.fator_curto = ep->fator_curto;
        s_ecfg.tempo_deteccao_curto_ms = ep->tempo_deteccao_curto_ms;
        electric_fsm_init(&s_electric_fsm, &s_ecfg);
    }
    if (fp) {
        feed_fsm_init(&s_feed_fsm, fp->feed_duration_min * 60U, fp->feed_cooldown_min * 60U);
    }
    if (rp) {
        restart_cfg_t rcfg = {
            .wait_time_ms = rp->tempo_espera_religamento_s * MS_PER_SEC,
            .stagger_interval_ms = rp->intervalo_religamento_s * MS_PER_SEC,
            .monitor_time_ms = rp->tempo_monitoramento_pos_relig_s * MS_PER_SEC
        };
        restart_fsm_init(&g_restart_fsm, &rcfg);
    }
    plug_manager_reload_limits();
}

static void on_config_changed(event_id_t evt, void *data, void *user_ctx)
{
    (void)evt;
    (void)data;
    (void)user_ctx;
    app_reload_fsms_from_config();
    ui_screen_manager_carousel_apply_config();
}

static void fill_safeoff_stabilization(safety_inputs_t *sin, bool cause_resolved, uint64_t now_ms)
{
    if (!cause_resolved) {
        s_safeoff_resolved_at_ms = 0;
    } else if (s_safeoff_resolved_at_ms == 0) {
        s_safeoff_resolved_at_ms = now_ms;
    }
    sin->cause_resolved_at_ms = s_safeoff_resolved_at_ms;
}

static void fill_emergency_stabilization(safety_inputs_t *sin, bool emerg_resolved, uint64_t now_ms)
{
    if (!emerg_resolved) {
        s_emergency_resolved_at_ms = 0;
    } else if (s_emergency_resolved_at_ms == 0) {
        s_emergency_resolved_at_ms = now_ms;
    }
    sin->cause_resolved_at_ms = s_emergency_resolved_at_ms;
}

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
    ESP_ERROR_CHECK(global_state_init());
    global_state_bind(&g_gs);
    memset(&g_gs, 0, sizeof(g_gs));
    g_gs.system_state = SYSTEM_STATE_NORMAL;
    g_gs.safeoff_reason = SAFEOFF_REASON_NONE;
    g_gs.last_reset_reason = (uint32_t)esp_reset_reason();
    /* @requirement RF-DADOS-001 observation_mode: ativado após reset anômalo
     * (WDT/panic/brownout), indicando boot cauteloso pós-reset. */
    {
        esp_reset_reason_t rr = esp_reset_reason();
        g_gs.observation_mode = (rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT ||
                                 rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT ||
                                 rr == ESP_RST_BROWNOUT);
    }
    snprintf(g_gs.fw_version, sizeof(g_gs.fw_version), "%s", HW_FW_VERSION_STR);
    snprintf(g_gs.srs_version, sizeof(g_gs.srs_version), "%s", HW_SRS_VERSION_STR);
    snprintf(g_gs.config_schema_version, sizeof(g_gs.config_schema_version), "%s", HW_CONFIG_SCHEMA_VERSION_STR);

    const system_params_storage_t *sys = config_get_system();
    g_gs.wizard_completed = sys->wizard_completed;
    g_gs.monitor_only_mode = sys->monitor_only_mode;
    g_gs.feed_active = false;
    g_gs.feed_remaining_s = 0;
    g_gs.feed_state = FEED_STATE_IDLE;
    g_gs.health_check_interval_s = HW_HEALTH_CHECK_INTERVAL_S;
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
    if (ato_service_get_level_adc(level_adc) != ESP_OK) {
        circuit_breaker_record_failure(CB_BUS_SPI_ADC);
        return false;
    }
    circuit_breaker_record_success(CB_BUS_SPI_ADC);
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
        case SAFEOFF_REASON_ELECTRIC_TOTAL: {
            if (!g_gs.pzem_ok) return false;
            return g_pzem.power_w <= (float)s_ecfg.total_power_limit_w;
        }
        case SAFEOFF_REASON_OVERVOLTAGE: {
            if (!g_gs.pzem_ok) return false;
            return g_pzem.voltage_v <= (float)s_ecfg.overvoltage_limit_v;
        }
        case SAFEOFF_REASON_UNDERVOLTAGE: {
            if (!g_gs.pzem_ok) return false;
            return g_pzem.voltage_v >= (float)s_ecfg.undervoltage_limit_v;
        }
        case SAFEOFF_REASON_PLUG_SHORT: {
            if (!g_gs.pzem_ok) return false;
            return g_pzem.current_a <= (float)s_ecfg.per_plug_current_limit_a;
        }
        case SAFEOFF_REASON_MCP23017_FAIL:
            return circuit_breaker_is_available(CB_BUS_I2C);
        case SAFEOFF_REASON_WDT_RECOVERY:
            return wdt_stats_get_resets_24h() <= HW_WDT_RESET_MAX_24H;
        case SAFEOFF_REASON_CONFIG_INVALID:
            return g_gs.config_schema_version[0] != '\0';
        case SAFEOFF_REASON_MANUAL_CRITICAL:
        case SAFEOFF_REASON_FSM_INVALID:
            return false;
        case SAFEOFF_REASON_SELFTEST_FAIL:
            return g_gs.selftest_passed;
        default:
            return false;
    }
}

static void handle_safeoff_exit(uint64_t now_s, uint64_t now_ms)
{
    safety_inputs_t sin;
    memset(&sin, 0, sizeof(sin));
    sin.safeoff_cause_resolved = safeoff_cause_is_resolved();
    sin.all_sensors_valid = (g_gs.temp_ok && g_gs.ato_ok && g_gs.pzem_ok);
    sin.selftest_passed = g_gs.selftest_passed;
    sin.manual_ack_received = safe_state_ack_manual_received(&g_gs);
    fill_safeoff_stabilization(&sin, sin.safeoff_cause_resolved, now_ms);

    if (g_gs.restart_in_progress && restart_fsm_is_complete(&g_restart_fsm)) {
        bool degraded = (!g_gs.temp_ok || !g_gs.ato_ok || !g_gs.selftest_passed ||
                         health_aggregate() != HEALTH_OK);
        ESP_LOGW(TAG, "RESTART: sequencia completa -> %s", degraded ? "DEGRADED" : "NORMAL");
        if (degraded) {
            global_state_enter_degraded(&g_gs, "restart_complete");
        } else {
            global_state_enter_normal(&g_gs, "restart_complete");
        }
        g_gs.restart_in_progress = false;
        plug_manager_set_restart_mask(0);
        (void)safeoff_record_resolve_latest(now_s);
        audit_log_event(AUDIT_SAFE_OFF,
                         degraded ? "Restart sequence complete -> DEGRADED"
                                  : "Restart sequence complete -> NORMAL");
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
        {
            uint16_t blocked = 0;
            for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
                plug_model_t *p = plug_manager_get((plug_id_t)(i + 1));
                if (p && p->blocked) blocked |= (uint16_t)(1U << (i + 1));
            }
            restart_fsm_set_blocked_mask(&g_restart_fsm, blocked);
        }
        restart_fsm_start(&g_restart_fsm, now_ms);
    } else if (sin.safeoff_cause_resolved && sin.manual_ack_received && sin.all_sensors_valid) {
        ESP_LOGD(TAG, "SAFE_OFF exit aguardando estabilizacao (%llums)",
                 (unsigned long long)(now_ms - s_safeoff_resolved_at_ms));
    }
}

static void handle_emergency_exit(uint64_t now_s, uint64_t now_ms)
{
    const thermal_output_t *tout = thermal_fsm_get_output(&s_thermal_fsm);
    bool emerg_resolved;
    if (strcmp(g_gs.safeoff_source_alm, "ALM-029") == 0) {
        emerg_resolved = (wdt_stats_get_resets_24h() <= HW_WDT_RESET_MAX_24H);
    } else {
        emerg_resolved = !(tout && tout->force_emergency);
    }

    safety_inputs_t sin;
    memset(&sin, 0, sizeof(sin));
    sin.emergency_resolved = emerg_resolved;
    sin.all_sensors_valid = (g_gs.temp_ok && g_gs.ato_ok && g_gs.pzem_ok);
    sin.manual_ack_received = safe_state_ack_manual_received(&g_gs);
    fill_emergency_stabilization(&sin, emerg_resolved, now_ms);

    if (safety_controller_can_exit_emergency(&g_gs, &sin, now_s)) {
        ESP_LOGW(TAG, "EMERGENCY exit conditions met -> SAFE_OFF");
        s_safeoff_resolved_at_ms = 0;
        thermal_fsm_clear_over_temp_latch(&s_thermal_fsm);
        (void)safeoff_record_resolve_latest(now_s);
        global_state_enter_safeoff(&g_gs, SAFEOFF_REASON_THERMAL_EXTREME,
                                   "ALM-028", "emergency_exit", now_s);
    }
}

static void reset_handler_check(uint64_t now_ms)
{
    static bool s_was_up_held = false;
    static uint64_t s_up_held_start_ms = 0;

    reset_state_t rst = reset_handler_get_state();
    uint16_t adc_val = HW_ADC_MAX_COUNT;
    mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_KEYPAD, &adc_val);
    bool up_pressed = (adc_val >= HW_AD_KEYPAD_ADC_UP_THRESH_MIN && adc_val < HW_AD_KEYPAD_ADC_UP_THRESH_MAX);

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
        } else if (now_ms - s_up_held_start_ms >= HW_RESET_HOLD_MS) {
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
    *temp_c = HW_TEMP_DEFAULT_C;
    *ato_level = HW_ATO_DEFAULT_ADC;

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

    float ph_val = 0.0f;
    bool ph_ok = false;
    (void)ph_sensor_read(&ph_val, &ph_ok);
    (void)ph_val;
}

static void update_plug_currents(float *plug_currents)
{
    if (!circuit_breaker_is_available(CB_BUS_SPI_ADC)) return;
    uint8_t ok = 0;
    for (uint8_t plug = 1; plug <= HW_RELAY_COUNT_MAX; plug++) {
        float cur = 0;
        if (acs712_read_plug(plug, &cur) == ESP_OK) ok++;
        plug_currents[plug - 1] = cur;
        /* RF-PLUG-004: alimenta detecção de bypass por plugue. */
        plug_manager_set_plug_current((plug_id_t)plug, cur);
    }
    /* @requirement RNF-ELECTRICAL-001 Realimenta o circuit breaker do barramento
     * SPI/ADC com o resultado real das leituras (antes só consultava, nunca gravava). */
    if (ok > 0) {
        circuit_breaker_record_success(CB_BUS_SPI_ADC);
    } else {
        circuit_breaker_record_failure(CB_BUS_SPI_ADC);
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
    static uint64_t s_last_energy_ms = 0;
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    uint64_t delta_ms = (s_last_energy_ms == 0) ? TASK_PERIOD_MS_SAFETY_CORE
                                                : (now_ms - s_last_energy_ms);
    s_last_energy_ms = now_ms;

    double total_wh = 0;
    float voltage = (g_pzem.valid && g_pzem.voltage_v > 0.0f) ? g_pzem.voltage_v
                                                              : (float)config_get_system()->mains_voltage;
    for (uint8_t p = 1; p <= HW_RELAY_COUNT_MAX; p++) {
        cdn_energy_update(p, plug_currents[p - 1], voltage, delta_ms);
        total_wh += cdn_energy_get_wh(p);
    }
    g_pzem.energy_wh = (float)total_wh;
}

static void update_feed_snapshot(uint64_t now_ms, uint64_t now_s)
{
    static uint64_t s_last_feed_snap_ms = 0;
    static feed_state_t s_prev_feed_state = FEED_STATE_IDLE;
    feed_state_t fst_now = feed_fsm_get_state(&s_feed_fsm);

    /* @requirement RF-FEED-001 (CA3) / RF-FSM-FEED-001 Feed Mode é abortado por
     * SAFE_OFF/EMERGENCY: encerra imediatamente, limpa snapshot e pedido pendente. */
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        if (feed_fsm_get_state(&s_feed_fsm) != FEED_STATE_IDLE) {
            feed_fsm_abort(&s_feed_fsm);
            feed_snapshot_clear();
            s_last_feed_snap_ms = 0;
            audit_log_event(AUDIT_FEED_MODE, "Feed mode aborted by SAFE_OFF/EMERGENCY");
            /* @requirement SRS §49 ALM-035 = "Feed Mode abortado por evento superior". */
            alert_manager_raise((int16_t)ALM_035, true, now_s);
        }
        g_feed_request = false;
        g_gs.feed_active = false;
        g_gs.feed_state = FEED_STATE_IDLE;
        g_gs.feed_remaining_s = 0;
        return;
    }

    feed_state_t fst = fst_now;
    if ((fst == FEED_STATE_ACTIVE || fst == FEED_STATE_COOLDOWN) &&
        (now_ms - s_last_feed_snap_ms > HW_FEED_SNAPSHOT_INTERVAL_MS)) {
        feed_snapshot_save(&s_feed_fsm, now_s, g_gs.time_valid);
        s_last_feed_snap_ms = now_ms;
    } else if (fst == FEED_STATE_IDLE && s_last_feed_snap_ms != 0) {
        feed_snapshot_clear();
        s_last_feed_snap_ms = 0;
    }

    if (g_feed_exit_request) {
        if (feed_fsm_get_state(&s_feed_fsm) != FEED_STATE_IDLE) {
            feed_fsm_abort(&s_feed_fsm);
            feed_snapshot_clear();
            audit_log_event(AUDIT_FEED_MODE, "Feed mode exited via UI");
        }
        g_feed_exit_request = false;
    }

    feed_state_t prev_feed = s_prev_feed_state;

    if (g_feed_request && g_gs.system_state == SYSTEM_STATE_NORMAL) {
        if (feed_fsm_start(&s_feed_fsm, now_ms)) {
            /* @requirement RF-PLUG-006 Captura o estado real dos plugues e a máscara
             * de bombas na BORDA de ativação e persiste o snapshot imediatamente
             * (não apenas no ciclo periódico). */
            s_feed_fsm.pre_feed_on_mask = plug_manager_get_on_mask();
            s_feed_fsm.pumps_off_mask = plug_manager_get_pump_mask();
            feed_snapshot_save(&s_feed_fsm, now_s, g_gs.time_valid);
            s_last_feed_snap_ms = now_ms;
            audit_log_event(AUDIT_FEED_MODE, "Feed mode started");
            g_feed_ui_pending = true;
        }
        g_feed_request = false;
    }

    fst_now = feed_fsm_get_state(&s_feed_fsm);
    if ((prev_feed == FEED_STATE_ACTIVE || prev_feed == FEED_STATE_COOLDOWN) &&
        fst_now == FEED_STATE_IDLE) {
        uint32_t restore = s_feed_fsm.pre_feed_on_mask & s_feed_fsm.pumps_off_mask;
        for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
            if (restore & (1U << i)) {
                plug_manager_toggle((plug_id_t)(i + 1), true);
            }
        }
    }
    s_prev_feed_state = fst_now;

    g_gs.feed_active = (fst_now == FEED_STATE_ACTIVE);
    g_gs.feed_state = fst_now;
    g_gs.feed_remaining_s = feed_fsm_remaining_s(&s_feed_fsm, now_ms);
}

static void update_led_signaling(uint64_t now_ms)
{
    static uint64_t s_last_blink_ms = 0;
    static bool s_blink_on = false;

    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        if (now_ms - s_last_blink_ms > HW_LED_BLINK_INTERVAL_MS) {
            s_blink_on = !s_blink_on;
            s_last_blink_ms = now_ms;
            if (s_blink_on) led_all_on();
            else led_all_off();
        }
    } else if (g_gs.feed_active) {
        /* @requirement RF-FEED-003 LED amarelo piscando durante Feed Mode. */
        if (now_ms - s_last_blink_ms > HW_LED_BLINK_INTERVAL_MS) {
            s_blink_on = !s_blink_on;
            s_last_blink_ms = now_ms;
            if (s_blink_on) led_set(LED_YELLOW);
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
    (void)now_s;
    const thermal_output_t *tout = thermal_fsm_get_output(&s_thermal_fsm);
    bool thermal_sensor_fault = (tout && tout->sensor_fault);

    if (thermal_sensor_fault && !g_gs.hw_alert_pending
        && g_gs.system_state < SYSTEM_STATE_SAFE_OFF
        && g_gs.system_state != SYSTEM_STATE_EMERGENCY) {
        g_gs.hw_alert_pending = true;
        g_gs.hw_alert_alm_id = ALM_013;
        snprintf(g_gs.hw_alert_msg, sizeof(g_gs.hw_alert_msg),
                 "Sensor de temperatura sem comunicacao. Toque para continuar.");
        audit_log_event(AUDIT_SAFE_OFF, "Temp sensor fail -> alerta critico (hw_alert)");
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
    /* @requirement RF-FSM-THERMAL-001/RF-FSM-ATO-001/RF-ENERGY-007 As zonas de
     * alerta das FSMs (térmico ALERT, ATO ERROR, elétrico em consumo alto/sobre-sub
     * tensão) elevam o sistema a DEGRADED — antes da escalada para SAFE_OFF na zona
     * crítica. Falha de sensor térmico também é DEGRADED (com ACK do usuário). */
    bool thermal_degraded = (tout && (tout->state == THERMAL_STATE_ALERT
                                      || tout->state == THERMAL_STATE_SENSOR_FAIL
                                      || tout->sensor_fault));
    bool ato_degraded = (aout && (aout->state == ATO_STATE_ERROR
                                  || aout->state == ATO_STATE_BLOCKED));
    bool electric_degraded = (eout && (eout->state == ELECTRIC_STATE_HIGH_CONSUMPTION
                                       || eout->state == ELECTRIC_STATE_OVERVOLTAGE
                                       || eout->state == ELECTRIC_STATE_UNDERVOLTAGE
                                       || eout->state == ELECTRIC_STATE_SENSOR_FAIL));
    sin.degraded_condition = (thermal_degraded || ato_degraded || electric_degraded);
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

    if (tout && tout->force_emergency) sin.emergency_source_alm = "ALM-028";
    else if (tout && tout->force_safe_off && !thermal_sensor_fault) sin.safeoff_source_alm = "ALM-026";
    else if (aout && aout->force_safe_off) sin.safeoff_source_alm = "ALM-037";
    else if (eout && eout->force_safe_off) {
        switch (eout->suggested_alm) {
            case ALM_050: sin.safeoff_source_alm = "ALM-050"; break;
            case ALM_051: sin.safeoff_source_alm = "ALM-051"; break;
            case ALM_052: sin.safeoff_source_alm = "ALM-052"; break;
            default:      sin.safeoff_source_alm = "ALM-055"; break;
        }
    }

    safety_controller_evaluate(&g_gs, &sin, now_s);

    if (g_gs.system_state != *prev_state) {
        static const char *state_names[] = {"NORMAL","DEGRADED","SAFE_OFF","EMERGENCY"};
        const char *from_s = (*prev_state < SYSTEM_STATE_COUNT) ? state_names[*prev_state] : "?";
        const char *to_s = (g_gs.system_state < SYSTEM_STATE_COUNT) ? state_names[g_gs.system_state] : "?";
        audit_log_state_change(from_s, to_s, sin.transition_cause);
        if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF) {
            s_safeoff_resolved_at_ms = 0;
            ui_screen_manager_carousel_pause();
        } else if (g_gs.system_state == SYSTEM_STATE_EMERGENCY) {
            s_emergency_resolved_at_ms = 0;
            ui_screen_manager_carousel_pause();
        } else if (*prev_state >= SYSTEM_STATE_SAFE_OFF && g_gs.system_state < SYSTEM_STATE_SAFE_OFF) {
            ui_screen_manager_carousel_resume();
        }
        *prev_state = g_gs.system_state;
    }

    if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF && g_gs.safeoff_reason != SAFEOFF_REASON_NONE) {
        int16_t alm = safeoff_reason_to_alm_id(g_gs.safeoff_reason);
        if (alm > 0) {
            alert_manager_raise((uint16_t)alm, true, now_s);
        }
    }
}

/* @requirement RF-TIME-003 Sincroniza NTP após WiFi e mantém flags wifi_ok/time_valid. */
static bool s_ntp_sync_requested = false;
static uint64_t s_last_time_tick_s = 0;

static void update_connectivity_and_time(uint64_t now_s)
{
    bool wifi_connected = wifi_ctl_sta_is_connected();
    g_gs.wifi_ok = wifi_connected;

    if (wifi_connected && !time_is_ntp_synced() && !s_ntp_sync_requested) {
        (void)time_sync_ntp_async(NULL);
        s_ntp_sync_requested = true;
        ESP_LOGI(TAG, "NTP sync solicitado (WiFi conectado)");
    }
    if (!wifi_connected) {
        s_ntp_sync_requested = false;
    }

    g_gs.time_valid = time_is_valid();

    if (g_gs.time_valid) {
        time_t ts = 0;
        if (time_get(&ts) == ESP_OK) {
            struct tm tm_info;
            if (localtime_r(&ts, &tm_info) != NULL) {
                cdn_energy_tick(tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);
            }
        }
    }

    if (s_last_time_tick_s == 0) {
        s_last_time_tick_s = now_s;
    } else if (now_s > s_last_time_tick_s) {
        time_manager_tick((uint32_t)(now_s - s_last_time_tick_s));
        s_last_time_tick_s = now_s;
    }
}

static void task_safety_core_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "safety_core task iniciada");

    system_state_t prev_state = g_gs.system_state;

    static uint32_t s_heartbeat_check_cycle = 0;

    while (1) {
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / USEC_PER_MSEC);
        const uint64_t now_s = now_ms / MS_PER_SEC;

        wdt_advanced_reset(TASK_ID_SAFETY_CORE);
        watchdog_guard_heartbeat(TASK_ID_SAFETY_CORE);

        circuit_breaker_update();
        event_bus_process_pending();
        health_matrix_update();
        update_connectivity_and_time(now_s);

        s_heartbeat_check_cycle++;
        if (s_heartbeat_check_cycle % HW_HEARTBEAT_CHECK_CYCLE_INTERVAL == 0) {
            for (int i = 0; i < TASK_ID_COUNT; i++) {
                if (i == TASK_ID_SAFETY_CORE) continue;
                if (i == TASK_ID_DIAG && !watchdog_guard_get_heartbeat(i)) continue;
                watchdog_status_t st = watchdog_guard_check(i);
                if (st.alarmed) {
                    if (st.severity == WATCHDOG_SEVERITY_CRITICAL) {
                        /* @requirement RF-WDT-RECOVERY-001 Travamento de task crítica:
                         * SAFE_OFF + relés OFF imediatos + observation_mode + ALM-043. */
                        ESP_LOGE(TAG, "Task %d critica sem heartbeat -> SAFE_OFF", i);
                        global_state_enter_safeoff(&g_gs, SAFEOFF_REASON_WDT_RECOVERY,
                                                   "ALM-043", "task_hang", now_s);
                        plug_manager_apply_safe_off();
                        g_gs.observation_mode = true;
                        const char *tn = task_manager_get_name(i);
                        alert_manager_raise_full(ALM_043, ALERT_SEVERITY_CRITICAL,
                                                 ALERT_CATEGORY_SYSTEM,
                                                 "Task critica sem heartbeat", 0.0f,
                                                 tn ? tn : "unknown",
                                                 0, true, true, now_s);
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

        reset_handler_tick(now_ms);
        reset_handler_check(now_ms);

        if (reset_handler_is_pending()) {
            wdt_advanced_reset(TASK_ID_SAFETY_CORE);
            feed_fsm_update(&s_feed_fsm, now_ms);
            ui_lvgl_tick(pdMS_TO_TICKS(20));
            vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_SAFETY_CORE));
            continue;
        }

        if (g_gs.system_state == SYSTEM_STATE_EMERGENCY) {
            handle_emergency_exit(now_s, now_ms);
        }
        if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF) {
            handle_safeoff_exit(now_s, now_ms);
        }

        float temp_c = HW_TEMP_DEFAULT_C;
        int32_t ato_level = HW_ATO_DEFAULT_ADC;
        float plug_currents[HW_RELAY_COUNT_MAX] = {0};

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
            vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_SAFETY_CORE));
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
        ein.plug_count = HW_RELAY_COUNT_MAX;
        ein.pzem_total_current_a = g_pzem.current_a;
        ein.voltage_v = g_pzem.voltage_v;
        ein.frequency_hz = g_pzem.frequency_hz;
        ein.pf = g_pzem.pf;
        ein.now_ms = now_ms;
        memcpy(ein.plug_currents_a, plug_currents, sizeof(ein.plug_currents_a));
        electric_fsm_update(&s_electric_fsm, &ein);

        static electric_state_t s_prev_electric = ELECTRIC_STATE_NORMAL;
        const electric_output_t *eout = electric_fsm_get_output(&s_electric_fsm);
        if (eout && eout->state != s_prev_electric) {
            switch (eout->state) {
                case ELECTRIC_STATE_OVERVOLTAGE:
                    electric_log_event(ELECTRIC_LOG_OVERVOLTAGE, 0, ein.voltage_v, "FSM");
                    break;
                case ELECTRIC_STATE_UNDERVOLTAGE:
                    electric_log_event(ELECTRIC_LOG_UNDERVOLTAGE, 0, ein.voltage_v, "FSM");
                    break;
                case ELECTRIC_STATE_OVERLOAD:
                    electric_log_event(ELECTRIC_LOG_OVERPOWER, 0, ein.total_power_w, "FSM");
                    break;
                case ELECTRIC_STATE_SHORT_CIRCUIT:
                    electric_log_event(ELECTRIC_LOG_PLUG_SHORT, eout->shorted_plug_id,
                                       ein.plug_currents_a[eout->shorted_plug_id > 0 ? eout->shorted_plug_id - 1 : 0], "FSM");
                    break;
                default:
                    break;
            }
            s_prev_electric = eout->state;
        }

        const thermal_output_t *tout = thermal_fsm_get_output(&s_thermal_fsm);
        const ato_output_t *aout = ato_fsm_get_output(&s_ato_fsm);

        update_safety_outputs(plug_currents, tout, aout, eout, now_s, now_ms, &prev_state);
        check_hw_alert(now_s);

        /* @requirement RF-FSM-THERMAL-001 / RF-FSM-ATO-001 ALMs das FSMs com clear simétrico. */
        fsm_alm_sync(tout ? (int16_t)tout->suggested_alm : 0, &s_last_tout_alm,
                     tout && !tout->force_safe_off && !tout->force_emergency, now_s);
        fsm_alm_sync(aout ? (int16_t)aout->suggested_alm : 0, &s_last_aout_alm, true, now_s);
        {
            int16_t e_alm = 0;
            if (eout && eout->suggested_alm != 0 && eout->suggested_alm != ALM_053
                && !eout->force_safe_off) {
                e_alm = (int16_t)eout->suggested_alm;
            }
            fsm_alm_sync(e_alm, &s_last_eout_alm, true, now_s);
        }

        if (eout && eout->state == ELECTRIC_STATE_SHORT_CIRCUIT && eout->shorted_plug_id > 0) {
            plug_manager_set_blocked((plug_id_t)eout->shorted_plug_id, true);
        }

        alm_monitor_tick(now_s);
        maintenance_mode_tick(now_s);
        g_gs.active_alerts_count = alert_manager_active_count();
        g_gs.critical_alerts_count = alert_manager_critical_count();
        {
            const security_params_storage_t *sec = config_get_security();
            uint32_t ack_to = (sec && sec->ack_timeout_s > 0)
                                  ? sec->ack_timeout_s : PARAM_SECURITY_DEFAULT_ACK_TIMEOUT_S;
            alert_manager_check_ack_timeout(now_s, ack_to);
        }

        /* @requirement RF-ALERT-006 Buzzer automático (respeita silence por ALM). */
        if (g_gs.critical_alerts_count > 0) {
            bool any_unsilenced = false;
            for (int16_t id = 1; id <= 65; id++) {
                if (!alert_manager_is_active(id)) continue;
                const alert_slot_t *sl = alert_manager_get_slot(id);
                if (sl && sl->severity >= ALERT_SEVERITY_CRITICAL
                    && !alert_manager_is_silenced(id, now_s)) {
                    any_unsilenced = true;
                    break;
                }
            }
            if (any_unsilenced) {
                buzzer_led_alert();
            }
        }
        g_gs.uptime_s = now_s;

        update_energy_accumulators(plug_currents);
        feed_fsm_update(&s_feed_fsm, now_ms);
        update_feed_snapshot(now_ms, now_s);

        plug_manager_set_thermal_request(PLUG_ID_P01,
            tout ? tout->request_heater_on : false,
            tout ? tout->request_cooler_on : false);
        plug_manager_set_ato_request(aout ? aout->pump_request_on : false);
        plug_manager_tick(now_s, g_gs.system_state, g_gs.feed_active);

        /* @requirement RF-DADOS-001 Publica o estado autoritativo (FSMs do laço)
         * para os serviços de leitura (UI/API), evitando instâncias divergentes. */
        {
            float total_current_a = 0.0f;
            for (uint8_t i = 0; i < HW_RELAY_COUNT_MAX; i++) {
                total_current_a += plug_currents[i];
            }
            thermal_service_publish(g_gs.temp_filtered_c, g_gs.temp_ok,
                                    tout ? tout->request_heater_on : false,
                                    tout ? tout->request_cooler_on : false);
            ato_service_publish(aout ? aout->pump_request_on : false,
                                aout ? (aout->state == ATO_STATE_OVERFLOW) : false);
            electric_service_publish(g_pzem.power_w, total_current_a, g_pzem.voltage_v,
                                     eout ? (eout->state == ELECTRIC_STATE_OVERLOAD) : false);
        }

        if (now_s % g_gs.health_check_interval_s == 0) {
            g_gs.last_health_check_timestamp = now_s;
            g_gs.sd_ok = storage_sd_is_mounted();
            /* @requirement RF-HEALTH-MATRIX-001 Alimenta a matriz de saúde a partir
             * dos indicadores já consolidados em g_gs (sem inventar fontes). */
            health_report(SUB_SENSOR_TEMP, g_gs.temp_ok ? HEALTH_OK : HEALTH_FAILED, NULL);
            health_report(SUB_SENSOR_LEVEL, g_gs.ato_ok ? HEALTH_OK : HEALTH_FAILED, NULL);
            health_report(SUB_SENSOR_CURRENT, g_gs.pzem_ok ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_SENSOR_VOLTAGE, g_gs.pzem_ok ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_SD, g_gs.sd_ok ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_WIFI, g_gs.wifi_ok ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_RTC, g_gs.time_valid ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_RELAY_BOARD, relay_mcp23017_ok() ? HEALTH_OK : HEALTH_FAILED, NULL);
            /* @requirement RF-HEALTH-MATRIX-001 Cobertura completa dos subsistemas.
             * Barramentos refletem o circuit breaker; 1-Wire segue o sensor de temp;
             * NVS é pré-requisito de boot (OK). PH/FLOW: presença física não confirmada
             * no SRS deste produto → UNKNOWN (não inventar OK/FAILED). */
            health_report(SUB_BUS_SPI,
                circuit_breaker_is_available(CB_BUS_SPI_ADC) ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_BUS_I2C,
                circuit_breaker_is_available(CB_BUS_I2C) ? HEALTH_OK : HEALTH_FAILED, NULL);
            health_report(SUB_BUS_1WIRE, g_gs.temp_ok ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_NVS, HEALTH_OK, NULL);
            health_report(SUB_SENSOR_PH,
                ph_sensor_is_ok() ? HEALTH_OK : HEALTH_DEGRADED,
                ph_sensor_is_ok() ? NULL : "leitura pH indisponivel");
            health_report(SUB_SENSOR_FLOW, HEALTH_UNKNOWN, "sensor nao presente neste produto");
            health_report(SUB_UI, g_gs.ui_ok ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            health_report(SUB_WEB_SECURITY,
                api_auth_has_password() ? HEALTH_OK : HEALTH_DEGRADED, NULL);
            {
                uint32_t wdt_resets = wdt_stats_get_resets_24h();
                health_report(SUB_WDT,
                    wdt_resets <= HW_WDT_RESET_MAX_24H ? HEALTH_OK : HEALTH_DEGRADED,
                    wdt_resets > HW_WDT_RESET_MAX_24H ? "wdt_resets_24h alto" : NULL);
            }
            health_matrix_update();

            /* @requirement RNF-ELECTRICAL-001 Integra os barramentos SD e I2C ao
             * circuit breaker a partir de indicadores já disponíveis. */
            if (g_gs.sd_ok) {
                circuit_breaker_record_success(CB_BUS_SPI_SD);
            } else {
                circuit_breaker_record_failure(CB_BUS_SPI_SD);
            }
            if (relay_mcp23017_ok()) {
                circuit_breaker_record_success(CB_BUS_I2C);
            } else {
                circuit_breaker_record_failure(CB_BUS_I2C);
            }
        }

        log_energy_if_due(now_s);
        update_led_signaling(now_ms);

        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_SAFETY_CORE));
    }
}

static void task_sensors_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "sensors task iniciada");

    float temp_c = HW_TEMP_DEFAULT_C;
    int32_t ato_level = HW_ATO_DEFAULT_ADC;
    float plug_currents[HW_RELAY_COUNT_MAX] = {0};

    while (1) {
        wdt_advanced_reset(TASK_ID_SENSORS);
        watchdog_guard_heartbeat(TASK_ID_SENSORS);

        read_sensors(&temp_c, &ato_level);
        update_plug_currents(plug_currents);
        read_pzem();

        /* @requirement RF-THERMAL-002 NÃO sobrescrever temp_filtered_c com a leitura
         * bruta: read_sensors() já atualiza a média móvel (janela 3) via temp_filter. */

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

        plug_manager_tick(g_gs.uptime_s, g_gs.system_state, g_gs.feed_active);

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

        cdn_energy_log_to_sd();
        storage_sd_tick_schedules();

        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_STORAGE));
    }
}

static void task_diag_fn(void *pv)
{
    (void)pv;
    ESP_LOGI(TAG, "diag task iniciada");

    uint64_t last_health_s = 0;

    while (1) {
        wdt_advanced_reset(TASK_ID_DIAG);
        watchdog_guard_heartbeat(TASK_ID_DIAG);

        uint64_t now_s = (uint64_t)(esp_timer_get_time() / USEC_PER_SEC);
        if (now_s - last_health_s >= g_gs.health_check_interval_s) {
            health_matrix_update();
            g_gs.last_health_check_timestamp = now_s;
            g_gs.sd_ok = storage_sd_is_mounted();
            if (g_gs.maintenance_mode) {
                (void)auth_recovery_sd_process(true);
            }
            last_health_s = now_s;
        }

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
    maintenance_mode_init();
    init_global_state();
    safety_controller_init(&g_gs);
    {
        const restart_params_storage_t *rp = config_get_restart();
        restart_cfg_t rcfg = {
            .wait_time_ms = rp->tempo_espera_religamento_s * MS_PER_SEC,
            .stagger_interval_ms = rp->intervalo_religamento_s * MS_PER_SEC,
            .monitor_time_ms = rp->tempo_monitoramento_pos_relig_s * MS_PER_SEC
        };
        restart_fsm_init(&g_restart_fsm, &rcfg);
    }
    alert_manager_init();
    alm_monitor_init();
    self_test_init();
    ESP_ERROR_CHECK(safeoff_record_init());
    wdt_stats_init((uint64_t)(esp_timer_get_time() / USEC_PER_SEC));

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
        for (uint8_t p = 1; p <= HW_RELAY_COUNT_MAX; p++) {
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
    if (ph_sensor_init() != ESP_OK) {
        ESP_LOGW(TAG, "Sensor pH (GPIO%d) nao inicializado", PIN_PH_ADC_GPIO);
    }
    ds3231_init();
    buzzer_led_init();
    cdn_energy_init();
    electric_log_init();
    profile_manager_init();

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
            .temp_min_c = tp->temp_min_c,
            .temp_max_c = tp->temp_max_c,
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
    temp_filter_init(HW_TEMP_FILTER_WINDOW);
    plug_manager_init();
    plug_manager_apply_preset(PLUG_ID_P03, PLUG_PRESET_BOMBA_ATO);
    const feed_params_storage_t *fp = config_get_feed();
    feed_fsm_init(&s_feed_fsm, fp->feed_duration_min * 60, fp->feed_cooldown_min * 60);
    {
        uint64_t boot_ms = (uint64_t)(esp_timer_get_time() / USEC_PER_MSEC);
        esp_err_t snap_err = feed_snapshot_restore(&s_feed_fsm, 0, boot_ms, g_gs.time_valid);
        if (snap_err == ESP_OK && s_feed_fsm.state != FEED_STATE_IDLE) {
            ESP_LOGW(TAG, "Feed snapshot restored (state=%d)", (int)s_feed_fsm.state);
            audit_log_event(AUDIT_FEED_MODE, "Feed state restored from NVS snapshot");
        }
    }

    esp_err_t ui_err = driver_ili9488_init();
    if (ui_err != ESP_OK) {
        ESP_LOGW(TAG, "Display LVGL nao disponivel (sistema continua sem UI)");
    } else {
        /* @requirement RF-UI-INPUT-001 Registro dos dispositivos de entrada LVGL.
         * Touch (XPT2046) e keypad analógico só podem ser registrados após
         * lv_init()/disp register feitos em driver_ili9488_init(). */
        esp_err_t touch_err = driver_xpt2046_init();
        if (touch_err != ESP_OK) {
            ESP_LOGW(TAG, "Touch LVGL nao registrado: %s", esp_err_to_name(touch_err));
        }
        esp_err_t keypad_err = driver_ad_keypad_lvgl_init();
        if (keypad_err != ESP_OK) {
            ESP_LOGW(TAG, "Keypad LVGL nao registrado: %s", esp_err_to_name(keypad_err));
        } else {
            ui_app_register_keypad();
        }
    }
    /* @requirement RNF-RESILIENCE-001 LVGL thread-safety */
    ESP_ERROR_CHECK(ui_lvgl_mutex_init());
    ui_app_init();

    esp_netif_init();
    esp_err_t api_err = api_rest_init();
    if (api_err != ESP_OK) {
        ESP_LOGW(TAG, "API web nao iniciou (sistema continua sem API)");
    }
    auth_recovery_sd_init();

    event_bus_init();
    event_bus_subscribe(EVENT_ID_CONFIG_CHANGED, on_config_changed, NULL);
    health_matrix_init();

    ESP_ERROR_CHECK(storage_facade_init());

    log_ctl_init();
    disp_ctl_init();
    sensor_ctl_init();
    wifi_ctl_init();

    thermal_service_init();
    ato_service_init();
    electric_service_init();
    time_manager_init();
    pending_actions_init();
    sec_policy_init();

    ESP_LOGI(TAG, "Init completo - lancando tasks normativas");

    wdt_advanced_init();
    watchdog_guard_init();

    /* @requirement RF-WDT-001..005 Consome o wdt_timeout_ms por task: o guard avalia
     * heartbeats a cada (HW_HEARTBEAT_CHECK_CYCLE_INTERVAL × período do safety_core),
     * convertendo o timeout configurado em nº de ciclos perdidos tolerados. */
    {
        const uint32_t check_interval_ms =
            (uint32_t)HW_HEARTBEAT_CHECK_CYCLE_INTERVAL * TASK_PERIOD_MS_SAFETY_CORE;
        for (int i = 0; i < TASK_ID_COUNT; i++) {
            const task_definition_t *td = task_manager_get_definition(i);
            if (td && td->wdt_timeout_ms > 0 && check_interval_ms > 0) {
                uint32_t thr = td->wdt_timeout_ms / check_interval_ms;
                if (thr < 1) thr = 1;
                watchdog_guard_set_threshold(i, thr);
            }
        }
    }

    /* @requirement RF-FLOW-BOOT-001/003 / RF-FLOW-SELFTEST-001 Gate de liberação:
     * os relés já estão em OFF seguro (relay_init_safe, acima) e NENHUMA task de
     * controle (safety_core/sensors/plug_control) foi lançada ainda. As construções
     * thermal/ato/electric_fsm_init apenas carregam parâmetros — não atuam relés.
     * O self-test roda AQUI, antes do task_manager_launch_all(); falha crítica entra
     * em SAFE_OFF antes da liberação, mantendo os relés desenergizados. */
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
        uint64_t now_s = (uint64_t)(esp_timer_get_time() / USEC_PER_SEC);
        if (!g_gs.hw_ok) {
            ESP_LOGE(TAG, "CRITICAL SELF-TEST FALHOU! Hardware critico com problemas -> SAFE_OFF + ALM-063");
            global_state_enter_safeoff(&g_gs, SAFEOFF_REASON_SELFTEST_FAIL, "ALM-063",
                                       "self-test critical fail", now_s);
            /* Desenergiza imediatamente, sem aguardar o 1º tick do plug_control. */
            plug_manager_apply_safe_off();
            alert_manager_raise_full(ALM_063, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_SYSTEM,
                                     "Falha CRITICA de hardware na inicializacao", 0.0f,
                                     "Hardware critico nao detectado. Sistema em SAFE_OFF.",
                                     0, true, false, now_s);
            audit_log_event(AUDIT_SELF_TEST, "Self-test CRITICAL falhou -> SAFE_OFF (ALM-063)");
        } else {
            ESP_LOGE(TAG, "SELF-TEST FALHOU (nao critico)! Hardware com problemas -> DEGRADED + alerta");
            /* @requirement ALM-063 / self_test.c: falhas CRITICAS -> SAFE_OFF (bloco acima);
             * falhas INFO (display/touch/etc.) -> DEGRADED operacional com ALM-063 HIGH. */
            global_state_enter_degraded(&g_gs, "selftest_fail");
            g_gs.hw_alert_pending = true;
            g_gs.hw_alert_alm_id = ALM_063;
            snprintf(g_gs.hw_alert_msg, sizeof(g_gs.hw_alert_msg),
                     "Falha de hardware na inicializacao. Toque para continuar em modo degradado.");
            alert_manager_raise_full(ALM_063, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                                     g_gs.hw_alert_msg, 0.0f,
                                     "Hardware nao detectado. Sistema operara sem protecoes.",
                                     0, true, false, now_s);
            audit_log_event(AUDIT_SELF_TEST, "Self-test nao critico falhou -> DEGRADED (hw alert)");
        }
    }

    reset_handler_init();

    ESP_LOGI(TAG, "FASE 7 - Lancando tasks via task_manager");

    task_manager_register_fn(TASK_ID_SAFETY_CORE, task_safety_core_fn);
    task_manager_register_fn(TASK_ID_SENSORS, task_sensors_fn);
    task_manager_register_fn(TASK_ID_PLUG_CONTROL, task_plug_control_fn);
    task_manager_register_fn(TASK_ID_STORAGE, task_storage_fn);
    task_manager_register_fn(TASK_ID_UI, task_ui_fn);
    task_manager_register_fn(TASK_ID_WEB, task_web_fn);
    task_manager_register_fn(TASK_ID_DIAG, task_diag_fn);
    task_manager_launch_all();

    vTaskDelete(NULL);
}
