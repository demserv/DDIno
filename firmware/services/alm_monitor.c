#include "alm_monitor.h"

#include "alert_manager.h"
#include "alm_ids.h"
#include "global_state.h"
#include "health_matrix.h"
#include "driver_pzem.h"
#include "config_manager.h"
#include "hardware_config.h"
#include "circuit_breaker.h"
#include "plug_model.h"
#include "plug_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "alm_monitor";

static uint64_t s_boot_ts = 0;

extern pzem_data_t g_pzem;

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
    if (condition) {
        const alert_slot_t *slot = alert_manager_get_slot(alm_id);
        if (slot && slot->active) {
            alert_manager_clear(alm_id);
        }
    }
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

    /* ALM-004: Reset por Brownout */
    if (!s_already_raised[3]) {
        if (esp_reset_reason() == ESP_RST_BROWNOUT) {
            raise_alm(ALM_004, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                      "Reset por Brownout detectado", "Verificar alimentacao", true, now_s);
        }
        s_already_raised[3] = true;
    }

    /* ALM-005: NVS invalida */
    if (!s_already_raised[4] && gs->config_schema_version[0] == '\0') {
        raise_alm(ALM_005, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                  "NVS invalida ou recovery aplicado", "Reconfigurar sistema e validar backup", true, now_s);
        s_already_raised[4] = true;
    }

    /* ALM-006: SD indisponivel */
    raise_alm(ALM_006, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
              "Cartao SD indisponivel", "Verificar cartao SD", false, now_s);
    clear_alm(ALM_006, gs->sd_ok);

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

    /* ALM-025: Orcamento mensal excedido (dados indisponiveis) */
    clear_alm(ALM_025, true);

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

    /* ALM-047: Falha do MCP3208 */
    if (health_get(SUB_SENSOR_LEVEL) == HEALTH_FAILED && !s_already_raised[46]) {
        raise_alm(ALM_047, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_SYSTEM,
                  "Falha do MCP3208/ADC externo", "Verificar SPI e alimentacao", true, now_s);
        s_already_raised[46] = true;
    }

    /* ALM-049: Falha de calibracao */
    if (health_get(SUB_SENSOR_VOLTAGE) == HEALTH_FAILED && !s_already_raised[48]) {
        raise_alm(ALM_049, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                  "Falha de calibracao/inconsistencia de sensor", "Recalibrar sensor", true, now_s);
        s_already_raised[48] = true;
    }

    /* ALM-050 + ALM-051: Tensao de rede (supplements electric_fsm) */
    {
        static uint64_t s_last_volt_check = 0;
        if (now_s - s_last_volt_check >= 10) {
            s_last_volt_check = now_s;
            if (g_pzem.valid) {
                const electric_params_storage_t *ep = config_get_electric();
                if (g_pzem.voltage_v > ep->overvoltage_limit_v) {
                    raise_alm(ALM_050, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_PROCESS,
                              "Sobretensao de rede", "Verificar rede eletrica", true, now_s);
                } else {
                    clear_alm(ALM_050, g_pzem.voltage_v <= ep->overvoltage_limit_v);
                }
                if (g_pzem.voltage_v < ep->undervoltage_limit_v && g_pzem.voltage_v > 0) {
                    raise_alm(ALM_051, ALERT_SEVERITY_HIGH, ALERT_CATEGORY_PROCESS,
                              "Subtensao de rede", "Verificar rede eletrica", true, now_s);
                } else {
                    clear_alm(ALM_051, g_pzem.voltage_v >= ep->undervoltage_limit_v || g_pzem.voltage_v <= 0);
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

    /* ALM-056: Falha ao religar plugue (dados indisponiveis) */
    {
        clear_alm(ALM_056, true);
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
            raise_alm(ALM_064, ALERT_SEVERITY_INFO, ALERT_CATEGORY_SYSTEM,
                      "Modo de manutencao ativo", "Finalizar ou revisar manutencao", false, now_s);
        }
        s_was_maintenance = maint_now;
    }
}
