// @requirement RF-ALERT-001 Monitor periódico de ALMs de sistema e processo
// @requirement RF-ALERT-003 Publicação/clear condicional sem flap (ALM-006 debounce)
// @requirement RF-ENERGY-003 ALM-025 orçamento de energia
// @requirement RF-PH-003 pH fora de calibração → ALM-049 (canônico); ALM-038/039 são ATO
// @requirement RF-PLUG-001 ALM-041/056 divergência relé e falha de acionamento
// @requirement RF-TIME-003 ALM-009/010 WiFi (via g_gs.wifi_ok)
#include "alm_monitor.h"

#include "alert_manager.h"
#include "alm_ids.h"
#include "global_state.h"
#include "health_matrix.h"
#include "driver_pzem.h"
#include "config_manager.h"
#include "hardware_config.h"
#include "circuit_breaker.h"
#include "services/storage_sd.h"
#include "plug_model.h"
#include "plug_manager.h"
#include "cdn_energy.h"
#include "drivers/driver_ph_sensor.h"
#include "driver_relay.h"
#include "wdt_stats.h"
#include "safety_controller.h"
#include "plug_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "alm_monitor";

static uint64_t s_boot_ts = 0;

extern pzem_data_t g_pzem;
extern global_state_t g_gs;

static void raise_alm(int16_t alm_id, alert_severity_t sev, alert_category_t cat,
                      const char *msg, const char *hint, bool ack_req, uint64_t now_s)
{
    const alert_slot_t *slot = alert_manager_get_slot(alm_id);
    if (!slot || !slot->active) {
        alert_manager_raise_full(alm_id, sev, cat, msg, 0.0f, hint, 0, ack_req, false, now_s);
    }
}

static void clear_alm(int16_t alm_id, bool condition)
{
    alert_manager_try_auto_clear(alm_id, condition);
}

esp_err_t alm_monitor_init(void)
{
    s_boot_ts = esp_timer_get_time() / 1000000ULL;
    ESP_LOGI(TAG, "Alarm monitor initialized");
    return ESP_OK;
}

void alm_monitor_tick(uint64_t now_s)
{
    const global_state_t *gs = global_state_get_snapshot_ptr();
    if (!gs) return;

    static bool s_already_raised[65] = {false};

    /* ALM-001: Boot bem-sucedido (once) */
    if (!s_already_raised[0] && now_s >= s_boot_ts + 2) {
        raise_alm(ALM_001, ALERT_SEVERITY_INFO, ALERT_CATEGORY_SYSTEM,
                  "Sistema inicializado com sucesso", "", false, now_s);
        s_already_raised[0] = true;
    }

    /* ALM-002: Reset por Power-On */
    if (!s_already_raised[1]) {
        if (esp_reset_reason() == ESP_RST_POWERON) {
            raise_alm(ALM_002, ALERT_SEVERITY_INFO, ALERT_CATEGORY_SYSTEM,
                      "Reset por Power-On", "", false, now_s);
        }
        s_already_raised[1] = true;
    }

    /* ALM-003: Reset por Watchdog (SRS §49 — HIGH/DEGRADED/ACK) */
    if (!s_already_raised[2]) {
        esp_reset_reason_t rr = esp_reset_reason();
        if (rr == ESP_RST_TASK_WDT || rr == ESP_RST_INT_WDT || rr == ESP_RST_WDT) {
            raise_alm(ALM_003, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                      "Reset por Watchdog detectado", "Verificar travamento do firmware", true, now_s);
        }
        s_already_raised[2] = true;
    }

    /* ALM-004: Reset por Brownout */
    if (!s_already_raised[3]) {
        if (esp_reset_reason() == ESP_RST_BROWNOUT) {
            raise_alm(ALM_004, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                      "Reset por Brownout detectado", "Verificar alimentacao", true, now_s);
        }
        s_already_raised[3] = true;
    }

    /* ALM-029: Excesso de resets → EMERGENCY (SRS §49). */
    {
        static bool s_alm029_raised = false;
        if (!s_alm029_raised && wdt_stats_get_resets_24h() > HW_WDT_RESET_MAX_24H) {
            raise_alm(ALM_029, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_SYSTEM,
                      "Excesso de resets na janela de 24h", "Verificar firmware e energia", true, now_s);
            s_alm029_raised = true;
            if (g_gs.system_state < SYSTEM_STATE_EMERGENCY) {
                global_state_enter_emergency(&g_gs, "ALM-029", "alm_monitor", now_s);
                plug_manager_apply_safe_off();
            }
        }
    }

    /* ALM-005: NVS invalida */
    if (!s_already_raised[4] && gs->config_schema_version[0] == '\0') {
        raise_alm(ALM_005, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                  "NVS invalida ou recovery aplicado", "Reconfigurar sistema e validar backup", true, now_s);
        s_already_raised[4] = true;
    }

    /* ALM-006: SD indisponivel */
    if (!gs->sd_ok) {
        raise_alm(ALM_006, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                  "Cartao SD indisponivel", "Verificar cartao SD", false, now_s);
    } else {
        clear_alm(ALM_006, true);
    }

    /* ALM-007: Espaco livre no SD abaixo de 10% */
    if (gs->sd_ok) {
        uint64_t total = 0, free = 0;
        if (storage_sd_get_space(&total, &free) == ESP_OK && total > 0) {
            bool low_space = (free * 100ULL) / total < 10ULL;
            if (low_space) {
                raise_alm(ALM_007, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                          "Espaco livre no SD abaixo de 10%", "Liberar espaco", false, now_s);
            } else {
                clear_alm(ALM_007, true);
            }
        }
    } else {
        clear_alm(ALM_007, true);
    }

    /* ALM-008: Heap/RAM livre abaixo do minimo */
    {
        static uint64_t s_last_heap_check = 0;
        if (now_s - s_last_heap_check >= 30) {
            s_last_heap_check = now_s;
            size_t free_heap = esp_get_free_heap_size();
            if (free_heap < 32768) {
                raise_alm(ALM_008, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                          "Heap livre abaixo do minimo", "Reiniciar e investigar consumo", false, now_s);
            } else {
                clear_alm(ALM_008, free_heap >= 32768);
            }
        }
    }

    /* ALM-009 + ALM-010: WiFi */
    {
        static bool s_wifi_was_ok = true;
        bool wifi_now = gs->wifi_ok;
        if (s_wifi_was_ok && !wifi_now) {
            raise_alm(ALM_009, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                      "WiFi desconectado", "Verificar rede LAN", false, now_s);
        }
        if (!s_wifi_was_ok && wifi_now) {
            clear_alm(ALM_009, true);
            raise_alm(ALM_010, ALERT_SEVERITY_INFO, ALERT_CATEGORY_SYSTEM,
                      "WiFi reconectado", "", false, now_s);
        }
        s_wifi_was_ok = wifi_now;
    }

    /* ALM-014: Sensor de temperatura restaurado */
    {
        static bool s_temp_was_bad = false;
        bool temp_bad_now = !gs->temp_ok;
        if (s_temp_was_bad && !temp_bad_now) {
            raise_alm(ALM_014, ALERT_SEVERITY_INFO, ALERT_CATEGORY_PROCESS,
                      "Sensor de temperatura restaurado", "", false, now_s);
        }
        s_temp_was_bad = temp_bad_now;
    }

    /* ALM-021: ATO restaurado */
    {
        static bool s_ato_was_bad = false;
        bool ato_bad_now = !gs->ato_ok;
        if (s_ato_was_bad && !ato_bad_now) {
            raise_alm(ALM_021, ALERT_SEVERITY_INFO, ALERT_CATEGORY_PROCESS,
                      "ATO desbloqueado/restaurado", "", false, now_s);
        }
        s_ato_was_bad = ato_bad_now;
    }

    /* ALM-022 + ALM-023: PZEM communication */
    {
        static bool s_pzem_was_bad = false;
        bool pzem_bad_now = !gs->pzem_ok;
        if (!pzem_bad_now && s_pzem_was_bad) {
            clear_alm(ALM_022, true);
            raise_alm(ALM_023, ALERT_SEVERITY_INFO, ALERT_CATEGORY_PROCESS,
                      "PZEM restaurado", "", false, now_s);
        }
        if (pzem_bad_now) {
            raise_alm(ALM_022, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                      "Falha de comunicacao com PZEM", "Verificar comunicacao PZEM", false, now_s);
        }
        s_pzem_was_bad = pzem_bad_now;
    }

    /* ALM-024: Potencia acima do limite */
    {
        static uint64_t s_last_power_check = 0;
        if (now_s - s_last_power_check >= 10) {
            s_last_power_check = now_s;
            if (g_pzem.valid && g_pzem.power_w > 0) {
                const electric_params_storage_t *ep = config_get_electric();
                if (g_pzem.power_w > ep->total_power_limit_w) {
                    raise_alm(ALM_024, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                              "Potencia acima do limite configurado", "Revisar carga conectada", false, now_s);
                } else {
                    clear_alm(ALM_024, g_pzem.power_w <= ep->total_power_limit_w);
                }
            }
        }
    }

    /* ALM-025: Orcamento mensal de energia excedido */
    {
        static uint64_t s_last_budget_check = 0;
        if (now_s - s_last_budget_check >= 60) {
            s_last_budget_check = now_s;
            const electric_params_storage_t *ep = config_get_electric();
            float budget_wh = ep->total_power_limit_w * HW_ENERGY_MONTHLY_BUDGET_H;
            float month_wh = cdn_energy_get_wh_month();
            if (budget_wh > 0.0f && month_wh > budget_wh) {
                raise_alm(ALM_025, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                          "Orcamento mensal de energia excedido", "Rever consumo", false, now_s);
            } else {
                clear_alm(ALM_025, budget_wh <= 0.0f || month_wh <= budget_wh);
            }
        }
    }

    /* ALM-031: Sobrecorrente em plugue */
    {
        static uint64_t s_last_oc_check = 0;
        if (now_s - s_last_oc_check >= 10) {
            s_last_oc_check = now_s;
            bool overcurrent = false;
            if (g_pzem.valid) {
                const electric_params_storage_t *ep = config_get_electric();
                for (int i = 0; i < HW_RELAY_COUNT_MAX && !overcurrent; i++) {
                    plug_model_t *plug = plug_manager_get((plug_id_t)(i + 1));
                    if (plug && plug->current_a > ep->per_plug_current_limit_a) {
                        overcurrent = true;
                    }
                }
            }
            if (overcurrent) {
                raise_alm(ALM_031, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_PROCESS,
                          "Sobrecorrente em plugue", "Verificar carga do plugue", false, now_s);
            } else {
                clear_alm(ALM_031, !overcurrent);
            }
        }
    }

    /* ALM-032: Plugue bloqueado */
    {
        static uint64_t s_last_block_check = 0;
        if (now_s - s_last_block_check >= 30) {
            s_last_block_check = now_s;
            bool any_blocked = false;
            for (int i = 0; i < HW_RELAY_COUNT_MAX; i++) {
                plug_model_t *plug = plug_manager_get((plug_id_t)(i + 1));
                if (plug && plug->effective_state == PLUG_EFFECTIVE_STATE_BLOCKED) {
                    any_blocked = true;
                    break;
                }
            }
            if (any_blocked) {
                raise_alm(ALM_032, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_PROCESS,
                          "Plugue bloqueado por protecao", "Inspecionar plugue e carga", true, now_s);
            } else {
                clear_alm(ALM_032, !any_blocked);
            }
        }
    }

    /* ALM-033 + ALM-034: Feed mode */
    {
        static bool s_was_feeding = false;
        bool feeding_now = gs->feed_active;
        if (!s_was_feeding && feeding_now) {
            raise_alm(ALM_033, ALERT_SEVERITY_INFO, ALERT_CATEGORY_PROCESS,
                      "Feed Mode ativado", "", false, now_s);
        }
        if (s_was_feeding && !feeding_now) {
            raise_alm(ALM_034, ALERT_SEVERITY_INFO, ALERT_CATEGORY_PROCESS,
                      "Feed Mode finalizado", "", false, now_s);
        }
        s_was_feeding = feeding_now;
    }

    /* ALM-036: Inconsistencia PZEM/ACS */
    {
        static uint64_t s_last_cons_check = 0;
        if (now_s - s_last_cons_check >= 30) {
            s_last_cons_check = now_s;
            if (g_pzem.valid) {
                float total_acs = 0;
                for (int i = 0; i < HW_RELAY_COUNT_MAX; i++) {
                    plug_model_t *plug = plug_manager_get((plug_id_t)(i + 1));
                    if (plug) total_acs += plug->current_a;
                }
                float diff = (total_acs > 0) ? (g_pzem.current_a - total_acs) / total_acs : 0;
                bool mismatch = (diff > 0.3f || diff < -0.3f);
                if (mismatch) {
                    raise_alm(ALM_036, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                              "Inconsistencia entre medicoes PZEM/ACS", "Verificar calibracao/carga", false, now_s);
                } else {
                    clear_alm(ALM_036, !mismatch);
                }
            }
        }
    }

    /* @requirement RF-PH-002/003/004 (Adendo baseline pH v3.11) pH é telemetria:
     * fora da faixa de calibração configurável gera ALM-049 (canônico — SRS §49).
     * Faixa de advertência (warn_low/warn_high) gera log SD sem SAFE_OFF. */
    {
        static uint64_t s_last_ph_check = 0;
        static bool s_ph_warn_active = false;
        if (now_s - s_last_ph_check >= 30) {
            s_last_ph_check = now_s;
            const ph_params_storage_t *php = config_get_ph();
            float ph = 0.0f;
            bool ph_valid = false;
            if (php && php->enabled &&
                ph_sensor_read(&ph, &ph_valid) == ESP_OK && ph_valid) {
                bool out_calib = (ph < php->calib_min_ph || ph > php->calib_max_ph);
                bool out_warn = (ph < php->warn_low_ph || ph > php->warn_high_ph);
                if (out_calib) {
                    raise_alm(ALM_049, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                              "pH fora da faixa calibrada — recalibrar sensor", "Recalibrar sensor", true, now_s);
                } else if (health_get(SUB_SENSOR_VOLTAGE) != HEALTH_FAILED) {
                    clear_alm(ALM_049, true);
                }
                if (!out_calib && out_warn) {
                    if (!s_ph_warn_active) {
                        char line[SD_LOG_LINE_MAX_LEN];
                        snprintf(line, sizeof(line),
                                 "{\"ts\":%llu,\"event\":\"ph_warn\",\"ph\":%.2f,"
                                 "\"warn_low\":%.2f,\"warn_high\":%.2f}",
                                 (unsigned long long)now_s, (double)ph,
                                 (double)php->warn_low_ph, (double)php->warn_high_ph);
                        storage_sd_write_log(SD_LOG_TYPE_ALERT, line);
                        s_ph_warn_active = true;
                    }
                } else if (s_ph_warn_active) {
                    char line[SD_LOG_LINE_MAX_LEN];
                    snprintf(line, sizeof(line),
                             "{\"ts\":%llu,\"event\":\"ph_warn_clear\",\"ph\":%.2f}",
                             (unsigned long long)now_s, (double)ph);
                    storage_sd_write_log(SD_LOG_TYPE_ALERT, line);
                    s_ph_warn_active = false;
                }
            }
        }
    }

    /* ALM-047: Falha do MCP3208 (ADC externo) — distinto de ALM-049 (pH). */
    {
        bool mcp3208_fail = (health_get(SUB_SENSOR_LEVEL) == HEALTH_FAILED);
        if (mcp3208_fail) {
            raise_alm(ALM_047, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                      "Falha do MCP3208/ADC externo", "Verificar SPI e alimentacao", true, now_s);
        } else {
            clear_alm(ALM_047, true);
        }
    }

    /* ALM-040: Consumo alto por plugue */
    {
        static uint64_t s_last_high_cons_check = 0;
        if (now_s - s_last_high_cons_check >= 30) {
            s_last_high_cons_check = now_s;
            bool high = false;
            if (g_pzem.valid) {
                const electric_params_storage_t *ep = config_get_electric();
                float limit = ep->per_plug_current_limit_a * 0.8f;
                for (int i = 0; i < HW_RELAY_COUNT_MAX; i++) {
                    plug_model_t *plug = plug_manager_get((plug_id_t)(i + 1));
                    if (plug && plug->current_a > limit) {
                        high = true;
                        break;
                    }
                }
            }
            if (high) {
                raise_alm(ALM_040, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                          "Consumo alto em plugue", "Verificar equipamento", false, now_s);
            } else {
                clear_alm(ALM_040, !high);
            }
        }
    }

    /* ALM-042: SD degradado */
    {
        health_status_t sd_health = health_get(SUB_SD);
        bool sd_failing = (sd_health == HEALTH_FAILED || sd_health == HEALTH_DEGRADED);
        if (sd_failing && gs->sd_ok) {
            raise_alm(ALM_042, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                      "SD degradado/erro de escrita", "Verificar/substituir SD", true, now_s);
        } else {
            clear_alm(ALM_042, !sd_failing || !gs->sd_ok);
        }
    }

    /* ALM-044: Frequencia de rede fora da faixa */
    {
        static uint64_t s_last_freq_check = 0;
        if (now_s - s_last_freq_check >= 10) {
            s_last_freq_check = now_s;
            if (g_pzem.valid && g_pzem.frequency_hz > 0) {
                bool out_of_range = (g_pzem.frequency_hz < 57.0f || g_pzem.frequency_hz > 63.0f);
                if (out_of_range) {
                    raise_alm(ALM_044, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                              "Frequencia de rede fora da faixa", "Verificar qualidade da rede", false, now_s);
                } else {
                    clear_alm(ALM_044, !out_of_range);
                }
            }
        }
    }

    /* ALM-045: Falha da UI local */
    if (!gs->ui_ok) {
        raise_alm(ALM_045, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                  "Falha da UI local", "Usar fallback e verificar display/input", true, now_s);
    } else {
        clear_alm(ALM_045, gs->ui_ok);
    }

    /* ALM-048: Falha MCP23017 → SAFE_OFF (relés I2C comprometidos). */
    {
        static uint64_t s_last_mcp_check = 0;
        if (now_s - s_last_mcp_check >= 10) {
            s_last_mcp_check = now_s;
            if (!relay_mcp23017_ok()) {
                raise_alm(ALM_048, ALERT_SEVERITY_CRITICAL, ALERT_CATEGORY_SYSTEM,
                          "Falha do MCP23017/relés I2C", "Verificar I2C e acionamento", true, now_s);
                if (g_gs.system_state < SYSTEM_STATE_SAFE_OFF) {
                    global_state_enter_safeoff(&g_gs, SAFEOFF_REASON_MCP23017_FAIL,
                                               "ALM-048", "alm_monitor", now_s);
                    plug_manager_apply_safe_off();
                }
            } else {
                clear_alm(ALM_048, true);
            }
        }
    }

    /* ALM-050/051: dono único = electric_fsm + safeoff_alm_map (sem duplicar aqui). */

    /* ALM-058: Tendência de corrente total (pré-alarme WARNING). */
    {
        static uint64_t s_last_cur_trend_check = 0;
        static float s_prev_current_a = 0.0f;
        static uint64_t s_cur_trend_since_s = 0;
        if (now_s - s_last_cur_trend_check >= 10) {
            s_last_cur_trend_check = now_s;
            if (g_pzem.valid) {
                const electric_params_storage_t *ep = config_get_electric();
                float limit = ep->total_current_limit_a;
                const float m = HW_ELECTRIC_TREND_MARGIN_PCT;
                bool rising = (s_prev_current_a > 0.01f) &&
                              (g_pzem.current_a > s_prev_current_a * 1.02f);
                bool near_limit = (limit > 0.0f) &&
                                  (g_pzem.current_a >= limit * (1.0f - m)) &&
                                  (g_pzem.current_a < limit);
                if (rising && near_limit) {
                    if (s_cur_trend_since_s == 0) {
                        s_cur_trend_since_s = now_s;
                    }
                } else {
                    s_cur_trend_since_s = 0;
                }
                s_prev_current_a = g_pzem.current_a;
                bool trend_active = (s_cur_trend_since_s != 0) &&
                                    (now_s - s_cur_trend_since_s >= HW_ELECTRIC_TREND_TIME_S);
                if (trend_active) {
                    raise_alm(ALM_058, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                              "Tendencia de corrente total elevada", "Monitorar consumo", false, now_s);
                } else {
                    clear_alm(ALM_058, !trend_active);
                }
            }
        }
    }

    /* @requirement RF-ENERGY-010 ALM-057: Tendência de tensão (pré-alarme WARNING). */
    {
        static uint64_t s_last_vtrend_check = 0;
        if (now_s - s_last_vtrend_check >= 10) {
            s_last_vtrend_check = now_s;
            if (g_pzem.valid && g_pzem.voltage_v > 0.1f) {
                const electric_params_storage_t *ep = config_get_electric();
                const float m = HW_ELECTRIC_TREND_MARGIN_PCT;
                float ov = ep->overvoltage_limit_v;
                float uv = ep->undervoltage_limit_v;
                bool near_ov = (ov > 0.0f) &&
                               (g_pzem.voltage_v >= ov * (1.0f - m)) && (g_pzem.voltage_v < ov);
                bool near_uv = (uv > 0.0f) &&
                               (g_pzem.voltage_v <= uv * (1.0f + m)) && (g_pzem.voltage_v > uv);
                if (near_ov || near_uv) {
                    raise_alm(ALM_057, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                              "Tendencia de tensao para fora da faixa", "Monitorar rede", false, now_s);
                } else {
                    clear_alm(ALM_057, true);
                }
            }
        }
    }

    /* ALM-053: Fator de potencia baixo */
    {
        static uint64_t s_last_pf_check = 0;
        if (now_s - s_last_pf_check >= 30) {
            s_last_pf_check = now_s;
            if (g_pzem.valid && g_pzem.pf > 0) {
                const electric_params_storage_t *ep = config_get_electric();
                if (g_pzem.pf < ep->pf_min) {
                    raise_alm(ALM_053, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_PROCESS,
                              "Fator de potencia baixo", "Verificar cargas indutivas", false, now_s);
                } else {
                    clear_alm(ALM_053, g_pzem.pf >= ep->pf_min);
                }
            }
        }
    }

    /* ALM-059: Circuit Breaker ativo */
    {
        static uint64_t s_last_cb_check = 0;
        if (now_s - s_last_cb_check >= 30) {
            s_last_cb_check = now_s;
            bool cb_active = false;
            for (int i = 0; i < CB_COUNT; i++) {
                if (circuit_breaker_get_state((cb_bus_id_t)i) == CB_STATE_OPEN) {
                    cb_active = true;
                    break;
                }
            }
            if (cb_active) {
                raise_alm(ALM_059, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                          "Sensor/subsistema em Circuit Breaker", "Verificar subsistema afetado", true, now_s);
            } else {
                clear_alm(ALM_059, !cb_active);
            }
        }
    }

    /* ALM-064: Modo de manutencao */
    {
        static bool s_was_maintenance = false;
        bool maint_now = gs->maintenance_mode;
        if (!s_was_maintenance && maint_now) {
            raise_alm(ALM_064, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                      "Modo de manutencao ativo", "Finalizar ou revisar manutencao", false, now_s);
        } else if (s_was_maintenance && !maint_now) {
            clear_alm(ALM_064, true);
        }
        s_was_maintenance = maint_now;
    }
}
