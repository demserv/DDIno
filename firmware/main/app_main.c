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

// @requirement RF-GLOBAL-001 a RF-GLOBAL-006 Estados globais
// @requirement RF-GLOBAL-SAFEOFF-EXIT-001 Saída segura de SAFE_OFF
// @requirement RF-GLOBAL-EMERG-EXIT-001 Saída controlada de EMERGENCY
// @requirement RF-FLOW-BOOT-003 Self-test falho → SAFE_OFF
// @requirement RF-PLUG-006.1 Restauração Feed Mode pós-queda
// @requirement RNF-CALIB-001 Calibração assistida no boot
#include "pin_map.h"
#include "global_state.h"
#include "hardware_config.h"
#include "hal_bus.h"
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
#include "fsm/thermal_fsm.h"
#include "drivers/driver_ds18b20.h"
#include "drivers/driver_ds3231.h"
#include "drivers/driver_buzzer_led.h"
#include "fsm/ato_fsm.h"
#include "web/api_rest.h"
#include "ui/ui_display.h"
#include "ui/ui_screens.h"
#include "core/circuit_breaker.h"
#include "services/self_test.h"
#include "services/wdt_advanced.h"
#include "services/temp_filter.h"
#include "services/audit_log.h"
#include "services/plug_manager.h"
#include "fsm/feed_fsm.h"
#include "fsm/restart_fsm.h"
#include "services/feed_snapshot.h"
#include "services/reset_handler.h"

static const char *TAG = "app_main";

global_state_t g_gs;
pzem_data_t g_pzem;
bool g_feed_request = false;
restart_fsm_t g_restart_fsm;

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
            const thermal_params_storage_t *tp = config_get_thermal();
            return g_gs.temp_filtered_c < tp->temp_critical_c;
        }
        case SAFEOFF_REASON_ATO_OVERFLOW: {
            if (!g_gs.ato_ok) return false;
            int32_t level_adc = 0;
            if (!read_ato_level(&level_adc)) return false;
            const ato_params_storage_t *ap = config_get_ato();
            int32_t overflow = ap->high_level_adc + ap->overflow_margin_adc;
            return level_adc <= overflow;
        }
        case SAFEOFF_REASON_ELECTRIC_TOTAL:
        case SAFEOFF_REASON_OVERVOLTAGE:
        case SAFEOFF_REASON_UNDERVOLTAGE: {
            if (!g_gs.pzem_ok) return false;
            const electric_params_storage_t *ep = config_get_electric();
            return g_pzem.power_w <= ep->total_power_limit_w;
        }
        case SAFEOFF_REASON_PLUG_SHORT: {
            if (!g_gs.pzem_ok) return false;
            const electric_params_storage_t *ep = config_get_electric();
            return g_pzem.current_a <= ep->per_plug_current_limit_a;
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
        g_gs.system_state = SYSTEM_STATE_SAFE_OFF;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Monitor Aquario Inteligente - Boot %s", g_gs.fw_version);
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

    const thermal_params_storage_t *tp = config_get_thermal();
    const electric_params_storage_t *ep = config_get_electric();
    const ato_params_storage_t *ap = config_get_ato();
    const selftest_params_storage_t *sp = config_get_selftest();
    electric_params_t ecfg = {
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
    thermal_params_t tcfg = {
        .temp_normal_c = tp->temp_normal_c,
        .temp_critical_c = tp->temp_critical_c,
        .temp_extreme_c = tp->temp_extreme_c,
        .hysteresis_c = tp->hysteresis_c,
        .extreme_enabled = tp->extreme_enabled
    };
    ato_params_t acfg = {
        .enabled = ap->enabled,
        .low_level_adc = ap->low_level_adc,
        .high_level_adc = ap->high_level_adc,
        .overflow_margin_adc = ap->overflow_margin_adc,
        .refill_timeout_s = ap->refill_timeout_s
    };

    thermal_fsm_t thermal_fsm;
    ato_fsm_t ato_fsm;
    electric_fsm_t electric_fsm;
    feed_fsm_t feed_fsm;
    thermal_fsm_init(&thermal_fsm, &tcfg);
    ato_fsm_init(&ato_fsm, &acfg);
    electric_fsm_init(&electric_fsm, &ecfg);
    temp_filter_init(5);
    plug_manager_init();
    const feed_params_storage_t *fp = config_get_feed();
    feed_fsm_init(&feed_fsm, fp->feed_duration_min * 60, 120);
    {
        uint64_t boot_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
        esp_err_t snap_err = feed_snapshot_restore(&feed_fsm, 0, boot_ms, g_gs.time_valid);
        if (snap_err == ESP_OK && feed_fsm.state != FEED_STATE_IDLE) {
            ESP_LOGW(TAG, "Feed snapshot restored (state=%d)", (int)feed_fsm.state);
            audit_log_event(AUDIT_FEED_MODE, "Feed state restored from NVS snapshot");
        }
    }

    esp_err_t ui_err = ui_display_init();
    if (ui_err != ESP_OK) {
        ESP_LOGW(TAG, "Display LVGL nao disponivel (sistema continua sem UI)");
    }
    ui_screens_init();

    esp_netif_init();
    esp_err_t api_err = api_rest_init();
    if (api_err != ESP_OK) {
        ESP_LOGW(TAG, "API web nao iniciou (sistema continua sem API)");
    }

    ESP_LOGI(TAG, "FASE 6 - API web /api/v1 + UI LVGL ativos");

    wdt_advanced_init();
    wdt_advanced_register(WDT_TASK_MAIN_LOOP, 2000);

    esp_err_t st = self_test_run_all(sp->selftest_timeout_ms);
    if (st != ESP_OK) {
        ESP_LOGW(TAG, "Self-test incompleto ou timeout: %s", esp_err_to_name(st));
    }
    self_test_log_results();
    g_gs.selftest_passed = self_test_all_passed();
    g_gs.hw_ok = self_test_critical_passed();

    if (!g_gs.selftest_passed) {
        ESP_LOGE(TAG, "SELF-TEST FALHOU! Entrando em SAFE_OFF");
        g_gs.system_state = SYSTEM_STATE_SAFE_OFF;
        g_gs.safeoff_reason = SAFEOFF_REASON_SELFTEST_FAIL;
        relay_all_off();
        alert_manager_raise(ALM_063, true, 0);
        audit_log_event(AUDIT_SELF_TEST, "Self-test falhou -> SAFE_OFF");
    }

    if (!g_gs.hw_ok) {
        ESP_LOGE(TAG, "CRITICAL HARDWARE SELF-TEST FALHOU! SAFE_FF");
        g_gs.system_state = SYSTEM_STATE_SAFE_OFF;
        g_gs.safeoff_reason = SAFEOFF_REASON_SELFTEST_FAIL;
        relay_all_off();
        alert_manager_raise(ALM_063, true, 0);
        audit_log_event(AUDIT_SELF_TEST, "Hardware critico falhou -> SAFE_OFF");
    }

    ESP_LOGI(TAG, "FASE 7 - Circuit Breaker, Self-test, WDT avancado ativos");

    reset_handler_init();

    system_state_t prev_state = g_gs.system_state;

    while (1) {
        circuit_breaker_update();
        wdt_advanced_reset(WDT_TASK_MAIN_LOOP);
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
        const uint64_t now_s = now_ms / 1000ULL;

        reset_handler_tick(now_ms);
        if (reset_handler_is_pending()) {
            wdt_advanced_reset(WDT_TASK_MAIN_LOOP);
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

        bool temp_valid = read_temp(&temp_c);
        g_gs.temp_ok = temp_valid;

        if (temp_valid) {
            g_gs.temp_filtered_c = temp_filter_update(temp_c);
        }

        thermal_input_t tin = {
            .sample_valid = temp_valid,
            .temp_c = temp_valid ? g_gs.temp_filtered_c : temp_c,
            .now_ms = now_ms
        };

        bool ato_valid = read_ato_level(&ato_level);
        g_gs.ato_ok = ato_valid;

        ato_input_t ain = {
            .sample_valid = ato_valid,
            .level_adc = ato_level,
            .now_ms = now_ms
        };

        if (!g_gs.wizard_completed) {
            wdt_advanced_reset(WDT_TASK_MAIN_LOOP);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        thermal_fsm_update(&thermal_fsm, &tin);
        ato_fsm_update(&ato_fsm, &ain);

        if (circuit_breaker_is_available(CB_BUS_SPI_ADC)) {
            for (uint8_t plug = 1; plug <= 10; plug++) {
                float cur = 0;
                acs712_read_plug(plug, &cur);
                plug_currents[plug - 1] = cur;
            }
        }

        if (circuit_breaker_is_available(CB_BUS_UART_PZEM)) {
            memset(&g_pzem, 0, sizeof(g_pzem));
            esp_err_t perr = pzem_read_all(&g_pzem);
            if (perr != ESP_OK) {
                circuit_breaker_record_failure(CB_BUS_UART_PZEM);
            } else {
                circuit_breaker_record_success(CB_BUS_UART_PZEM);
            }
        }

        g_gs.pzem_ok = g_pzem.valid;

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
        electric_fsm_update(&electric_fsm, &ein);

        const thermal_output_t *tout = thermal_fsm_get_output(&thermal_fsm);
        const ato_output_t *aout = ato_fsm_get_output(&ato_fsm);
        const electric_output_t *eout = electric_fsm_get_output(&electric_fsm);

        safety_inputs_t sin;
        memset(&sin, 0, sizeof(sin));
        sin.emergency_condition = (tout && tout->force_emergency);
        sin.safeoff_condition = (tout && tout->force_safe_off)
                             || (aout && aout->force_safe_off)
                             || (eout && eout->force_safe_off);
        sin.degraded_condition = false;
        sin.all_sensors_valid = (g_gs.temp_ok && g_gs.pzem_ok);
        sin.selftest_passed = g_gs.selftest_passed;
        sin.emergency_resolved = !sin.emergency_condition;
        sin.safeoff_cause_resolved = !sin.safeoff_condition;

        if (tout && tout->force_safe_off) {
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
        else if (tout && tout->force_safe_off) sin.safeoff_source_alm = "ALM-026";
        else if (aout && aout->force_safe_off) sin.safeoff_source_alm = "ALM-037";
        else if (eout && eout->force_safe_off) {
            sin.safeoff_source_alm = (eout->suggested_alm == ALM_052) ? "ALM-052" : "ALM-055";
        }

        safety_controller_evaluate(&g_gs, &sin, now_s);

        if (g_gs.system_state != prev_state) {
            static const char *state_names[] = {"NORMAL","DEGRADED","SAFE_OFF","EMERGENCY"};
            const char *from_s = (prev_state < 4) ? state_names[prev_state] : "?";
            const char *to_s = (g_gs.system_state < 4) ? state_names[g_gs.system_state] : "?";
            audit_log_state_change(from_s, to_s, sin.transition_cause);
            prev_state = g_gs.system_state;
        }

        if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF && g_gs.safeoff_reason != SAFEOFF_REASON_NONE) {
            int16_t alm = safeoff_reason_to_alm_id(g_gs.safeoff_reason);
            if (alm > 0) {
                alert_manager_raise(alm, true, now_s);
            }
        }

        g_gs.active_alerts_count = alert_manager_active_count();
        g_gs.critical_alerts_count = alert_manager_critical_count();
        g_gs.uptime_s = now_s;

        double total_wh = 0;
        float voltage = (g_pzem.valid && g_pzem.voltage_v > 0.0f) ? g_pzem.voltage_v : 127.0f;
        for (uint8_t p = 1; p <= 10; p++) {
            cdn_energy_update(p, plug_currents[p-1], voltage, 50);
            total_wh += cdn_energy_get_wh(p);
        }
        g_pzem.energy_wh = (float)total_wh;

        feed_fsm_update(&feed_fsm, now_ms);

        {
            static uint64_t s_last_feed_snap_ms = 0;
            feed_state_t fst = feed_fsm_get_state(&feed_fsm);
            if ((fst == FEED_STATE_ACTIVE || fst == FEED_STATE_COOLDOWN) &&
                (now_ms - s_last_feed_snap_ms > 5000)) {
                feed_snapshot_save(&feed_fsm, now_s, g_gs.time_valid);
                s_last_feed_snap_ms = now_ms;
            } else if (fst == FEED_STATE_IDLE && s_last_feed_snap_ms != 0) {
                feed_snapshot_clear();
                s_last_feed_snap_ms = 0;
            }
        }

        if (g_feed_request && g_gs.system_state == SYSTEM_STATE_NORMAL) {
            if (feed_fsm_start(&feed_fsm, now_ms)) {
                audit_log_event(AUDIT_FEED_MODE, "Feed mode started");
            }
            g_feed_request = false;
        }

        g_gs.feed_active = (feed_fsm_get_state(&feed_fsm) == FEED_STATE_ACTIVE);
        g_gs.feed_state = feed_fsm_get_state(&feed_fsm);
        g_gs.feed_remaining_s = feed_fsm_remaining_s(&feed_fsm);

        plug_manager_set_thermal_request(PLUG_ID_P02,
            tout ? tout->request_heater_on : false,
            tout ? tout->request_cooler_on : false);

        plug_manager_tick(now_s, g_gs.system_state, g_gs.feed_active);

        if (now_s % g_gs.health_check_interval_s == 0) {
            g_gs.last_health_check_timestamp = now_s;
            g_gs.sd_ok = storage_sd_is_mounted();
        }

        static uint64_t s_last_energy_log_s = 0;
        if (now_s - s_last_energy_log_s >= HW_SD_LOG_INTERVAL_S) {
            cdn_energy_log_to_sd();
            s_last_energy_log_s = now_s;
        }

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

        ui_screen_update_all();
        lv_timer_handler();

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
