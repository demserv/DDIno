# RTM — SRS × Código × Testes

## Convenção

| Coluna | Descrição |
|--------|-----------|
| ID SRS | Identificador do requisito na SRS v3.11-AF.3+AF.4+B12/N+B13/N |
| Descrição | Texto resumido fiel do requisito |
| Arquivo(s) | Caminho(s) dos arquivos que implementam |
| Função(ões) | Funções principais que realizam o requisito |
| Linhas | Números de linha relevantes |
| Evidência | O que comprova a implementação |
| Teste(s) | Identificador(es) do(s) teste(s) em TEST_PLAN.md |
| Status | COMPLIANT / PARTIAL / MISSING / N/A |

## Matriz

| ID SRS | Descrição | Arquivo(s) | Função(ões) | Evidência | Teste | Status |
|--------|-----------|------------|-------------|-----------|-------|--------|
| RF-GLOBAL-001 | Estados NORMAL/DEGRADED/SAFE_OFF/EMERGENCY | include/system_types.h | system_state_t | enum canônico 4 estados | TC-FSM-001 | COMPLIANT |
| RF-GLOBAL-002 | Transições com prioridade e log | core/global_state.c | global_state_transition | prev/next, audit, event_bus, antiflap | TC-FSM-TRANS-001 | COMPLIANT |
| RF-GLOBAL-003 | Sinais sonoros/visuais estado crítico | core/safety_controller.c | safety_controller_evaluate | SAFe_OFF/EMERGENCY aciona buzzer/LED via driver_buzzer_led | TC-SAFETY-SIGNAL-001 | COMPLIANT |
| RF-GLOBAL-004 | Rastreamento causa SAFE_OFF | include/global_state.h, services/safeoff_alm_map.c | safeoff_reason_t, safeoff_reason_to_alm_id | 13 razões mapeadas para ALMs | TC-SAFEOFF-REASON-001 | COMPLIANT |
| RF-GLOBAL-SAFEOFF-EXIT-001 | Pré-condições saída SAFe_OFF | core/safety_controller.c | safety_controller_can_exit_safeoff | sensores+ACK+10s estabilização | TC-SAFEOFF-EXIT-001 | COMPLIANT |
| RF-GLOBAL-EMERG-EXIT-001 | Saída controlada de EMERGENCY | core/safety_controller.c | safety_controller_can_exit_emergency | emergência resolvida+ACK+estabilização | TC-EMERG-EXIT-001 | COMPLIANT |
| RF-GLOBAL-005 | ViewModel centralizado sem hardcode | main/ui/hmi/ui_view_model.c | ui_view_model_update_from_system | Consome dados reais de serviços/FSM/health_matrix | TC-VIEW-MODEL-001 | COMPLIANT |
| RF-UI-DISPLAY-001 | TFT ILI9488 480×320 SPI | ui/ui_display.c | ui_display_init, ui_display_lvgl_flush | ILI9488 init+flush | TC-UI-DISPLAY-001 | COMPLIANT |
| RF-UI-CAROUSEL-001 | Carrossel com pausa SAFe_OFF | main/ui/hmi/ui_screen_manager.c | ui_screen_manager_tick | bloqueio+intervalo HW | TC-UI-CAROUSEL-001 | COMPLIANT |
| RF-UI-STATUS-001 | Topbar/footer persistentes | main/ui/hmi/ui_app.c | ui_app_tick | topbar+footer+overlay | TC-UI-STATUS-001 | COMPLIANT |
| RF-UI-ATO-001 | Tela ATO com estado/bomba/ADC | main/ui/hmi/screens/ui_screen_ato.c/h | ui_screen_ato_create, ui_screen_ato_update | Estados ATO visiveis, ADC ao vivo, bloqueio refill inseguro | TC-UI-ATO-001 | COMPLIANT |
| RF-UI-WIZARD-001 | Wizard inicial 6 etapas | main/ui/hmi/screens/ui_screen_wizard.c/h | wizard_render_step | Welcome→Password→Thermal→ATO→Electric→Review | TC-WIZARD-001 | COMPLIANT |
| RF-UI-WIZARD-002 | Wizard integrado ao ConfigManager | main/ui/hmi/screens/ui_screen_wizard.c, services/config_manager.c | wizard_finish_cb, config_set_wizard_step | Grava config_root via commit transactional | TC-WIZARD-CFG-001 | COMPLIANT |
| RF-UI-WIZARD-003 | Bloqueio operacional com Wizard incompleto | services/command_validator.c, services/safety_gate.c | command_validator_can_toggle_plug, safety_gate_can_enable_automation | wizard_completed=false bloqueia comandos | TC-WIZARD-BLOCK-001 | COMPLIANT |
| RF-UI-WIZARD-004 | Wizard exibe parâmetros reais | main/ui/hmi/screens/ui_screen_wizard.c | wizard_render_step | Steps thermal/ato/electric mostram config real do ConfigManager | TC-WIZARD-DISPLAY-001 | COMPLIANT |
| RF-UI-WIZARD-005 | Conclusão grava config validada | main/ui/hmi/screens/ui_screen_wizard.c | wizard_finish_cb | Commit transactional, marca wizard_completed=true | TC-WIZARD-FINISH-001 | COMPLIANT |
| RF-ENERGY-001 | PZEM Modbus V/A/W/kWh/PF/Hz | drivers/driver_pzem.c | pzem_read_all | CRC+timeout | TC-ENERGY-001 | COMPLIANT |
| RF-HW-UART-001 | PZEM UART GPIO17/18 9600 8N1 | drivers/driver_pzem.c, include/hardware_config.h | pzem_init | UART1 configurado com constantes canônicas | TC-HW-UART-001 | COMPLIANT |
| RF-PZEM-MODEL-001 | PZEM-004T v4.0 canônico | include/hardware_config.h | HW_PZEM_MODEL_VERSION | v4.0 em todas as referências normativas | TC-PZEM-MODEL-001 | COMPLIANT |
| RF-THERMAL-001 | FSM térmica com setpoint/histerese | fsm/thermal_fsm.c/h | thermal_fsm_update | 6 estados + histerese + latch overtemp | TC-THERMAL-001 | COMPLIANT |
| RF-THERMAL-002 | Limites críticos thermal_runaway | fsm/thermal_fsm.c/h | thermal_fsm_update | critical_high→CRITICAL→SAFE_OFF | TC-THERMAL-002 | COMPLIANT |
| RF-THERMAL-003 | Sensor DS18B20 | drivers/driver_ds18b20.c | ds18b20_read_temperature | Conversão+leitura 1-Wire | TC-THERMAL-SENSOR-001 | COMPLIANT |
| RF-THERMAL-004 | Filtro temp with rejeição spo | services/temp_filter.c/h | temp_filter_update | Média móvel+spo detect | TC-THERMAL-FILTER-001 | COMPLIANT |
| RF-THERMAL-005 | Tendência (°C/min) | fsm/thermal_fsm.c | thermal_fsm_update | Cálculo de tendência por janela temporal | TC-THERMAL-TREND-001 | COMPLIANT |
| RF-THERMAL-006 | Parâmetros térmicos configuráveis | services/config_manager.c, include/param_catalog.h | config_get_thermal | setpoint, histerese, limites via NVS | TC-THERMAL-CFG-001 | COMPLIANT |
| RF-THERMAL-007 | Dois estágios aquecimento/resfriamento | fsm/thermal_fsm.c | thermal_fsm_update | Heater/cooler com prioridade | TC-THERMAL-DUAL-001 | COMPLIANT |
| RF-THERMAL-008 | Heater priority sobre cooler | fsm/thermal_fsm.c | thermal_fsm_update | Heater liga antes do cooler | TC-THERMAL-PRIO-001 | COMPLIANT |
| RF-THERMAL-009 | Exclusão mútua heater/cooler | fsm/thermal_fsm.c | thermal_fsm_update | both ON→SAFE_OFF | TC-THERMAL-009 | COMPLIANT |
| RF-ATO-001 | Debounce temporal ADC ATO | fsm/ato_fsm.c/h | ato_fsm_update | 3 amostras com threshold | TC-ATO-DEBOUNCE-001 | COMPLIANT |
| RF-ATO-002 | FSM ATO 6 estados | fsm/ato_fsm.c/h | ato_fsm_update | NORMAL/REFILLING/OVERFLOW/BLOCKED/ERROR/DISABLED | TC-ATO-002 | COMPLIANT |
| RF-ATO-003 | Sensor nível água | services/ato_service.c, fsm/ato_fsm.c | ato_service_is_water_present | Leitura ADC com threshold configurável | TC-ATO-SENSOR-001 | COMPLIANT |
| RF-ATO-004 | Bomba ATO com proteções | services/ato_service.c, services/safety_gate.c | ato_service_set_pump, safety_gate_can_enable_automation | Overflow/blocked bloqueiam bomba | TC-ATO-PUMP-001 | COMPLIANT |
| RF-ATO-DIGITAL-001 | Calibração ATO | fsm/ato_fsm.c, services/config_manager.c | config_set_ato ato_params_storage_t | ADC zero offset via NVS | TC-ATO-CAL-001 | COMPLIANT |
| RF-PLUG-001 | Modelo P01-P10 | services/plug_manager.c/h | plug_manager_* | 10 plugs com estados+modos | TC-PLUG-001 | COMPLIANT |
| RF-PLUG-002 | Tomadas críticas P01/P02 | services/command_validator.c | command_validator_can_toggle_plug | Dupla confirmação para plugs 1-2 | TC-PLUG-CRITICAL-001 | COMPLIANT |
| RF-PLUG-003 | Múltiplos modos operacionais | services/plug_manager.c/h | plug_manager_set_mode | OFF/ON/AUTO/SCHEDULE | TC-PLUG-MODE-001 | COMPLIANT |
| RF-PLUG-004 | Bypass detection (ALM-054) | services/plug_manager.c | plug_manager_check_bypass | Corrente com plug OFF→ALM | TC-PLUG-BYPASS-001 | COMPLIANT |
| RF-PLUG-008 | Min ON/OFF time | services/plug_manager.c, include/param_catalog.h | plug_manager_can_toggle | HW_PLUG_MIN_ON_S, HW_PLUG_MIN_OFF_S | TC-PLUG-MINTIME-001 | COMPLIANT |
| RF-PLUG-011 | Dupla confirmação P01/P02 | services/command_validator.c | command_validator_can_toggle_plug | requires_double_confirmation=true para plugs 1-2 | TC-PLUG-DOUBLE-001 | COMPLIANT |
| RF-PLUG-012 | Relocation AQUECEDOR/COOLER | services/plug_manager.c | plug_manager_relocate | Trocas de role entre plugs | TC-PLUG-RELOC-001 | COMPLIANT |
| RF-ELETRIC-001 | FSM elétrica com 4 proteções | services/electric_fsm.c/h | electric_fsm_update | Sobretensão/subtensão/sobrecorrente/FP | TC-ELETRIC-001 | COMPLIANT |
| RF-ELETRIC-002 | Medição PZEM | drivers/driver_pzem.c | pzem_read_all | V/A/W/PF/Hz/kWh via Modbus | TC-ELETRIC-MEAS-001 | COMPLIANT |
| RF-ELETRIC-003 | Limites configuráveis | services/config_manager.c, include/param_catalog.h | config_get_electric | over_voltage, under_voltage, current_limit, pf_min | TC-ELETRIC-CFG-001 | COMPLIANT |
| RF-ELETRIC-004 | Perfil 127V/220V/bivolt | include/system_types.h, services/config_manager.c | config_electric_profile_t | 3 perfis com defaults diferentes | TC-ELETRIC-PROFILE-001 | COMPLIANT |
| RF-ENERGY-009 | PF enforcement (ALM-058) | services/electric_fsm.c | electric_fsm_update | PF abaixo do mínimo→ALM | TC-ENERGY-PF-001 | COMPLIANT |
| RF-ENERGY-010 | Log SD energia periódico | services/cdn_energy.c, main/app_main.c | log_energy_if_due, cdn_energy_log_csv | CSV com timestamp+medidas | TC-ENERGY-LOG-001 | COMPLIANT |
| RF-HEALTH-MATRIX-001 | Matriz de saúde 14 subsistemas | services/health_matrix.c/h | health_report, health_aggregate | OK/DEGRADED/FAILED/OPEN/HALF_OPEN/CLOSED | TC-HEALTH-001 | COMPLIANT |
| RF-HEALTH-MATRIX-010 | Agregacao saúde | services/health_matrix.c | health_aggregate | Any FAILED→OPEN, any DEGRADED→DEGRADED | TC-HEALTH-AGG-001 | COMPLIANT |
| RF-DATA-CONFIG-ROOT-001 | ConfigRoot unificado com CRC | services/config_root.c/h, services/config_manager.c | config_root_load/save, config_root_validate, config_root_commit, config_root_rollback | CRC32, schema version, commit/rollback transactional | TC-CFG-ROOT-001 | COMPLIANT |
| RF-DATA-CONSISTENCY-001 | bypess_detected / role_override_source | services/plug_manager.c, services/electric_fsm.c | plug_manager_check_bypass | Estado bypass rastreável | TC-DATA-CONSIST-001 | COMPLIANT |
| RF-STORAGE-001 | NVS namespaces config | services/storage_manager.c | storage_set_blob, storage_get_blob | 9 namespaces com schema version | TC-STORAGE-001 | COMPLIANT |
| RF-STORAGE-002 | RAM fallback (ring buffer) | services/storage_sd.c, services/storage_atomic.c | sd_log_with_fallback | 64 entradas RAM se SD indisponível | TC-STORAGE-RAM-001 | COMPLIANT |
| RF-STORAGE-010 | SD card logging | services/storage_sd.c/h | sd_log_write, atomic_write_and_rename | CSV com .tmp→rename | TC-STORAGE-SD-001 | COMPLIANT |
| RF-PERSIST-001 | Persistência NVS | services/storage_manager.c | storage_*, config_manager.* | Set/get/commit/blob para cada namespace | TC-PERSIST-001 | COMPLIANT |
| RF-PERSIST-ATOMIC-001 | Escrita atômica | services/storage_sd.c | atomic_write_and_rename | .tmp + fdatasync + rename | TC-PERSIST-ATOMIC-001 | COMPLIANT |
| RF-PERSIST-EXPORT-001 | Export/import NVS | services/storage_manager.c | storage_export_to_buffer, storage_import_from_buffer | Backup/restore namespace | TC-PERSIST-EXPORT-001 | COMPLIANT |
| RF-WEB-001 | API REST /api/v1 | web/api_rest.c | 27 handlers | Todos endpoints + auth + rate limit | TC-API-001 | COMPLIANT |
| RF-WEB-002 | Autenticação API | web/api_auth.c/h | api_auth_verify, api_auth_set_password | SHA-256 + token bearer | TC-API-AUTH-001 | COMPLIANT |
| RF-WEB-003 | Comandos validados | services/command_validator.c, services/command_dispatcher.c | command_validator_*, command_dispatch_execute | Validação antes da execução de todo comando | TC-API-CMD-001 | COMPLIANT |
| RF-WEB-004 | Rate limit | web/api_rate_limit.c/h | api_rate_limit_check | Por IP, configurável por minuto | TC-API-RATE-001 | COMPLIANT |
| RF-WEB-005 | Health endpoint | web/api_rest.c, services/health_matrix.c | api_health_handler, health_aggregate | GET /api/v1/health | TC-API-HEALTH-001 | COMPLIANT |
| RF-ALERT-001 | Sistema de ALM 65 IDs | services/alert_manager.c/h, include/alm_ids.h | alert_manager_raise, alert_manager_ack | Ciclo de vida: raise→ack→clear | TC-ALM-001 | COMPLIANT |
| RF-ALERT-002 | Silence enforcement | services/alert_manager.c/h | alert_manager_set_silenced | Habilita/desabilita alertas sonoros | TC-ALM-SILENCE-001 | COMPLIANT |
| RF-ALERT-004 | ACK timeout ALM-046 | services/alert_manager.c/h | alert_manager_check_ack_timeout | ALM crítico sem ACK→escalação | TC-ALM-ACK-001 | COMPLIANT |
| RF-FLOW-BOOT-001 | Sequência boot 15+ etapas | main/app_main.c | app_main | NVS→config→FSM→UI→tasks | TC-BOOT-001 | COMPLIANT |
| RF-FLOW-BOOT-003 | Self-test→SAFE_OFF | main/app_main.c, services/self_test.c | self_test_run | Se self-test falha→SAFE_OFF | TC-BOOT-SELFTEST-001 | COMPLIANT |
| RF-FLOW-BOOT-004 | .tmp orphan scan | services/storage_sd.c | sd_init | Limpeza .tmp após init SD | TC-BOOT-ORPHAN-001 | COMPLIANT |
| RF-FLOW-SELFTEST-001 | Self-test boot | services/self_test.c/h | self_test_run | MCP23017, SD, PZEM, RTC, DS18B20 | TC-SELFTEST-001 | COMPLIANT |
| RF-FLOW-FEED-001 | Modo alimentação | fsm/feed_fsm.c/h, services/feed_snapshot.c/h | feed_fsm_update, feed_snapshot_start | Temporizador+cooldown+pausa relés | TC-FEED-001 | COMPLIANT |
| RF-FLOW-RESTART-001 | Religamento progressivo | fsm/restart_fsm.c/h | restart_fsm_update | Plug a plug com intervalo+monitor | TC-RESTART-001 | COMPLIANT |
| RF-FLOW-RESTART-002 | Plug blocking pós-falha | fsm/restart_fsm.c/h | restart_fsm_update | Bitmask blocked_plugs | TC-RESTART-BLOCK-001 | COMPLIANT |
| RF-HW-RELAY-LOGIC-001 | Relés active-high boot OFF | drivers/driver_relay.c | relay_init_safe | GPIO5-6 (P01,P02) + MCP23017 (P03-P10) | TC-RELAY-001 | COMPLIANT |
| RF-HW-SPI-MUTEX-001 | Mutex SPI compartilhado | hal/hal_spi.c/h | hal_spi_transaction_* | TFT + SD + ADC + touch com mutex | TC-SPI-001 | COMPLIANT |
| RF-HW-I2C-001 | I2C GPIO8/9 MCP+RTC | hal/hal_bus.c | hal_bus_init | Endereços 0x20, 0x68 | TC-I2C-001 | COMPLIANT |
| RF-HW-ADC-001 | MCP3208 ADC SPI | drivers/driver_mcp3208.c | mcp3208_read | Leituras ADC para sensores | TC-ADC-001 | COMPLIANT |
| RF-HW-TOUCH-001 | Touch TFT | ui/ui_touch.c | ui_touch_init | Calibração+eventos LVGL | TC-TOUCH-001 | COMPLIANT |
| RF-HW-KEYPAD-001 | Teclado AD | drivers/driver_ad_keypad.c | ad_keypad_read | Leitura ADC 5 teclas | TC-KEYPAD-001 | COMPLIANT |
| RF-HW-DS3231-001 | RTC DS3231 I2C | drivers/driver_ds3231.c | ds3231_read_time, ds3231_set_time | Hora/minuto/segundo via I2C | TC-RTC-001 | COMPLIANT |
| RF-HW-BUZZER-001 | Buzzer/LED | drivers/driver_buzzer_led.c | buzzer_led_beep, buzzer_led_set | Saída MCP23017 | TC-BUZZER-001 | COMPLIANT |
| RF-HW-ACS712-001 | Corrente ACS712 por plug | drivers/driver_acs712.c | acs712_read_ma | Leitura ADC com zero offset | TC-ACS712-001 | COMPLIANT |
| RF-HW-DEVKIT-001 | DevKit ESP32-S3 | include/hardware_config.h, include/pin_map.h | HW_PLATFORM_NAME | ESP32-S3 240MHz | TC-DEVKIT-001 | COMPLIANT |
| RF-HW-OTA-001 | OTA update | partitions.csv | — | deferido por decisão arquitetural | TC-OTA-001 | N/A |
| RF-WDT-RECOVERY-001 | WDT recovery relés OFF | services/watchdog_guard.c, include/watchdog_guard.h | watchdog_guard_* | Heartbeat tasks + recovery | TC-WDT-001 | COMPLIANT |
| RF-WDT-RECOVERY-002 | Task WDT timeouts | include/task_manager.h | TASK_WDT_TIMEOUT_* | Timeout por task | TC-WDT-TASK-001 | COMPLIANT |
| RF-AUDIT-SCOPE-001 | Escopo auditoria definido | docs/AUDIT_SCOPE.md, docs/THIRD_PARTY_MANIFEST.md, docs/SBOM_MINIMAL.md, tools/concatena_auditoria.ps1 | — | managed_components excluído do corpus funcional | TC-AUDIT-SCOPE-001 | COMPLIANT |
| RF-SAFETY-GATE-001 | Central safety gate | services/safety_gate.c/h | safety_gate_can_enable_automation | Wizard+SAFE_OFF+selftest+hw_ok | TC-SAFETY-GATE-001 | COMPLIANT |
| RF-COMMAND-DISPATCH-001 | Central command dispatcher | services/command_dispatcher.c/h | command_dispatch_execute | Valida+executa comandos | TC-CMD-DISPATCH-001 | COMPLIANT |
| RF-SAFEOFF-RECORD-001 | Histórico SAFE_OFF persistente | services/safeoff_record.c/h | safeoff_record_append, safeoff_record_resolve_latest | NVS persist, ring de 16 entradas | TC-SAFEOFF-REC-001 | COMPLIANT |
| RF-LOG-MANAGER-001 | Log manager com ring buffer | services/log_manager.c/h, services/storage_atomic.c/h | log_manager_append, log_manager_get_recent | Ring buffer 256 entradas RAM | TC-LOG-MGR-001 | COMPLIANT |
| RF-UI-LOGS-001 | Tela de logs real | main/ui/hmi/screens/ui_screen_logs.c/h | ui_screen_logs_create, ui_screen_logs_update | Exibe últimos logs via log_manager | TC-UI-LOGS-001 | COMPLIANT |
| RF-LVGL-NORM-001 | LVGL sem exemplos/demos | sdkconfig.defaults | CONFIG_LV_BUILD_EXAMPLES=0, CONFIG_LV_BUILD_TEST=0 | Apenas biblioteca, sem upstream code | TC-LVGL-NORM-001 | COMPLIANT |

## Resumo

| Status | Quantidade |
|--------|------------|
| COMPLIANT | 78 |
| PARTIAL | 0 |
| MISSING | 0 |
| N/A | 1 (OTA) |
| **Total** | **79** |

## Legenda

- COMPLIANT: Implementado, testado, evidência disponível
- PARTIAL: Implementação incompleta ou sem teste
- MISSING: Não implementado
- N/A: Não aplicável (deferido por decisão)
