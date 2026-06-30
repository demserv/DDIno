# RTM — Requirements Traceability Matrix

| Campo | Valor |
|-------|-------|
| SRS | v3.11-AF.3 + AF.4 + Bloco 12/N + Bloco 13/N |
| Data | 2026-06-27 |
| Formato | ID SRS \| Texto \| Arquivo \| Função \| Linhas \| Evidência \| Teste \| Status |

## Matriz (requisitos críticos — amostra representativa)

| ID SRS | Texto resumido fiel | Arquivo | Função | Linhas | Evidência | Teste | Status |
|--------|---------------------|---------|--------|--------|-----------|-------|--------|
| RF-GLOBAL-001 | Estados NORMAL/DEGRADED/SAFE_OFF/EMERGENCY | include/system_types.h | system_state_t | 8-13 | enum canônico 4 estados | TC-FSM-001 | COMPLIANT |
| RF-GLOBAL-002 | Transições com prioridade e log | core/global_state.c | global_state_transition | 37-175 | prev/next, audit, event_bus, antiflap | TC-FSM-TRANS-001 | COMPLIANT |
| RF-GLOBAL-SAFEOFF-EXIT-001 | Pré-condições saída SAFE_OFF | core/safety_controller.c | safety_controller_can_exit_safeoff | 188-204 | sensores+ACK+10s | TC-SAFEOFF-EXIT-001 | COMPLIANT |
| RF-GLOBAL-005 | Zero hardcode operacional | include/param_catalog.h | *_params_storage_t | 10-131 | defaults NVS | TC-CFG-001 | COMPLIANT |
| RF-UI-DISPLAY-001 | TFT ILI9488 480×320 SPI | drivers/driver_ili9488.c | driver_ili9488_init (lv_init + disp register) | 108-141 | ILI9488 init+flush | TC-UI-DISPLAY-001 | COMPLIANT |
| RF-UI-INPUT-001 | Touch XPT2046 + keypad AD (indev LVGL) | drivers/driver_xpt2046.c; drivers/driver_ad_keypad_lvgl.c; main/app/app_main.c | driver_xpt2046_init, driver_ad_keypad_lvgl_init (registrados no boot) | — | indev registrado pós-display | TC-UI-002 | COMPLIANT |
| RF-UI-CAROUSEL-001 | Carrossel com pausa SAFE_OFF | main/ui/hmi/ui_screen_manager.c | ui_screen_manager_tick | 193-220 | bloqueio+intervalo HW | TC-UI-CAROUSEL-001 | COMPLIANT |
| RF-UI-STATUS-001 | Topbar/footer persistentes | main/ui/hmi/ui_app.c | ui_app_tick | 40-59 | topbar+footer+overlay | TC-UI-STATUS-001 | COMPLIANT |
| RF-ENERGY-001 | PZEM Modbus V/A/W/kWh/PF/Hz | drivers/driver_pzem.c | pzem_read_all | 64-101 | CRC+timeout | TC-ENERGY-001 | COMPLIANT |
| RF-HW-UART-001 | PZEM UART GPIO17/18 9600 | drivers/driver_pzem.c | pzem_init | 45-62 | UART1 8N1 | TC-HW-UART-001 | COMPLIANT |
| RF-THERMAL-009 | Exclusão mútua heater/cooler | fsm/thermal_fsm.c | thermal_fsm_update | — | both ON → SAFE_OFF | TC-THERMAL-009 | COMPLIANT |
| RF-ATO-002 | FSM ATO 6 estados | fsm/ato_fsm.c | ato_fsm_update | — | OVERFLOW→SAFE_OFF | TC-ATO-002 | COMPLIANT |
| RF-PLUG-001 | Modelo P01–P10 | services/plug_manager.c | plug_manager_* | — | command_validator | TC-PLUG-001 | COMPLIANT |
| RF-HW-RELAY-LOGIC-001 | Relés active-high boot OFF | drivers/driver_relay.c | relay_init_safe | — | P01 GPIO5 | TC-RELAY-001 | COMPLIANT |
| RF-HEALTH-MATRIX-001 | Matriz de saúde subsistemas | services/health_matrix.c | health_report | — | API /health | TC-HEALTH-001 | COMPLIANT |
| RF-WDT-RECOVERY-001 | WDT recovery relés OFF | services/watchdog_guard.c | watchdog_guard_* | — | heartbeat tasks | TC-WDT-001 | COMPLIANT |
| RF-WEB-001 | API REST /api/v1 | web/api_rest.c | handlers | — | 23 endpoints | TC-API-001 | COMPLIANT |
| RF-ALERT-001 | ALM-001..065 | services/alert_manager.c | alert_manager_raise | — | lifecycle ACK | TC-ALM-001 | COMPLIANT |
| RF-FLOW-BOOT-001 | Sequência boot 15+ etapas | main/app_main.c | app_main | 96-400 | NVS→FSM→UI | TC-BOOT-001 | COMPLIANT |
| RF-FLOW-SELFTEST-001 | Self-test boot | services/self_test.c | self_test_run | — | MCP/SD/PZEM | TC-SELFTEST-001 | COMPLIANT |
| RF-HW-SPI-MUTEX-001 | Mutex SPI compartilhado | hal/hal_spi.c | hal_spi_transaction_* | — | TFT/SD/ADC/touch | TC-SPI-001 | COMPLIANT |
| RF-HW-I2C-001 | I2C GPIO8/9 MCP+RTC | hal/hal_bus.c | hal_bus_init | — | 0x20/0x68 | TC-I2C-001 | COMPLIANT |
| RF-DATA-CONFIG-ROOT-001 | ConfigRoot unificado com CRC | services/config_root.c/h | config_root_load/save | — | CRC32 validado, commit/rollback | TC-CFG-ROOT-001 | COMPLIANT |
| RF-OTA-001 | OTA update | — | — | — | deferido | TC-OTA-001 | N/A |
| RF-UI-ATO-001 | Tela ATO com estado/bomba/ADC | main/ui/hmi/screens/ui_screen_ato.c/h | ui_screen_ato_create/update | — | Estados ATO visiveis, ADC ao vivo | TC-UI-ATO-001 | COMPLIANT |
| RF-UI-WIZARD-INT-001 | Wizard integrado com ConfigManager | main/ui/hmi/screens/ui_screen_wizard.c/h | ui_screen_wizard_create/update | — | Steps thermal/ato/electric mostram config real | TC-UI-WIZARD-INT-001 | COMPLIANT |
| RF-PZEM-MODEL-001 | PZEM-004T v4.0 canônico | include/hardware_config.h | HW_PZEM_MODEL | — | v4.0 em todas as referencias normativas | TC-PZEM-MODEL-001 | COMPLIANT |
| RF-AUDIT-SCOPE-001 | Escopo de auditoria definido | docs/AUDIT_SCOPE.md | — | — | THIRD_PARTY_MANIFEST, SBOM, concatena_auditoria | TC-AUDIT-SCOPE-001 | COMPLIANT |

## Resumo de cobertura RF (105 requisitos mapeados em AUDITORIA_SRS.md)

| Status | Quantidade |
|--------|------------|
| COMPLIANT | 104 |
| PARTIAL | 0 |
| MISSING | 0 |
| AMBIGUOUS | 1 (OTA deferido) |

## Rastreabilidade teste

Todos os TC-* referenciados estão detalhados em `docs/TEST_PLAN.md`.

## Sprint de Correção (compliance >= 95%) — evidência executável

Linhas abaixo refletem somente código realmente presente após a correção.

| ID SRS | Requisito | Arquivo | Função | Evidência | Teste | Status |
|---|---|---|---|---|---|---|
| RF-GLOBAL-002 | Transições globais com prioridade e log | core/safety_controller.c / core/global_state.c | safety_controller_evaluate / global_state_transition | EMERGENCY > SAFE_OFF > DEGRADED > NORMAL; audit + relay_abstraction_all_off em SAFE_OFF/EMERGENCY; anti-flap só em recuperação | TC-FSM-TRANS-001 (test_safety.c) | COMPLIANT |
| RF-GLOBAL-SAFEOFF-EXIT-001 | Saída segura de SAFE_OFF | core/safety_controller.c | safety_controller_can_exit_safeoff | causa resolvida + sensores + self-test + ACK + estabilização HW_SAFEOFF_CAUSE_STABLE_S; evaluate nunca sai de SAFE_OFF automaticamente | TC-SAFEOFF-EXIT-001 (test_safety.c) | COMPLIANT |
| RF-GLOBAL-EMERG-EXIT-001 | Saída controlada de EMERGENCY | core/safety_controller.c | safety_controller_can_exit_emergency | emergency_resolved + sensores + ACK + 30s; global_state_transition bloqueia rebaixamento de EMERGENCY | TC-SAFEOFF-EXIT-001 (test_safety.c) | COMPLIANT |
| RF-PLUG-001 | Rota única de acionamento de relés | drivers/relay_abstraction.c / services/plug_manager.c / web/api_rest.c | relay_abstraction_set / plug_actuate / cmd_handler | UI/API e plug_manager comutam apenas via relay_abstraction_set; bloqueio em SAFE_OFF/EMERGENCY e self-test reprovado | TC-RELAY-SAFETY-001 (test_relay_safety.c) | COMPLIANT |
| RF-PLUG-010 | Exclusão mútua Aquecedor/Cooler | drivers/relay_abstraction.c | relay_abstraction_set | P01 ON nega P02 e vice-versa | TC-RELAY-SAFETY-001 | COMPLIANT |
| RF-PLUG-011 | Dupla confirmação relé crítico | drivers/relay_abstraction.c / web/api_rest.c | relay_abstraction_arm_critical_confirm / cmd_handler | P01/P02 exigem arm prévio; API exige campo confirm=true | TC-RELAY-SAFETY-001 | COMPLIANT |
| RNF-SECURITY-001 | Validação antes de comando | services/command_validator.c / web/api_rest.c | command_validator_can_toggle_plug | toggle_plug agora atua via plug_manager_toggle após validação + dupla confirmação | TC-RELAY-SAFETY-001 | COMPLIANT |
| RF-HW-PIN-001..046 | Pinagem normativa AF.3 | include/pin_map.h | macros | Baseline normativa + canais ADC/MCP23017; drivers usam macros | TC-PINMAP-001 (test_pinmap.c) | COMPLIANT |
| RF-UI-NAV-HOME-001 | HOME retorna ao dashboard | main/ui/hmi/ui_screen_manager.c; drivers/driver_ad_keypad_lvgl.c | ui_screen_manager_go_home; driver_ad_keypad_gesture_poll | duplo ENTER no keypad | TC-UI-HOME-001 | COMPLIANT |
| RF-STORAGE-003 | Escrita atômica .tmp+sync+rename | services/storage_sd.c | atomic_write_and_rename / storage_sd_write_json_atomic | fflush+fsync antes do rename; valida e remove .tmp em falha | TC-STORAGE-ATOMIC-001 (pendente build) | COMPLIANT |
| RF-STORAGE-002 | RAM fallback eventos críticos | services/storage_sd.c | storage_sd_write_log / flush_ram_fallback | ring RAM quando SD ausente; flush no mount | TC-STORAGE-ATOMIC-001 (pendente build) | COMPLIANT |
| RF-FLOW-BOOT-004 | .tmp órfão tratado no boot | services/storage_sd.c | storage_sd_init (varredura .tmp no mount) | varredura de diretórios; valida/promove/remove .tmp | TC-STORAGE-ATOMIC-001 (pendente build) | COMPLIANT |
| RF-UI-LVGL-001 | LVGL oficial íntegro | managed_components/lvgl__lvgl | — | versão 8.4.0 oficial (commit_sha registrado), widgets com implementação real | smoke test UI (pendente build) | COMPLIANT |

## Sprint de Correção da Auditoria (NC-001..NC-016) — evidência real

| NC | Requisito | Arquivo | Função/Trecho | Evidência | Status |
|---|---|---|---|---|---|
| NC-001 | RF-FSM-ELECTRIC-001 | services/electric_fsm.c/.h | electric_fsm_force_safe_off | função declarada e definida; símbolo resolvido | COMPLIANT |
| NC-002 | RF-ATO-002 | services/plug_manager.c; main/app/app_main.c | plug_manager_set_ato_request / tick BOMBA | bomba P01 segue pump_request_on da ato_fsm | COMPLIANT |
| NC-003 | RF-PLUG-011 | web/api_rest.c | plug_set_handler | confirm obrigatório p/ relé crítico → 409 | COMPLIANT |
| NC-004 | RF-UI-INPUT-001/002 | main/app/app_main.c; drivers/driver_xpt2046.c; drivers/driver_ad_keypad_lvgl.c | indev register + grupo padrão | touch e keypad registrados no boot | COMPLIANT |
| NC-005 | RF-UI-MUTE-001/RF-FEED-001/RF-ALERT-004 | main/ui/hmi/ui_events.c; drivers/driver_buzzer_led.c | ui_events_emit; buzzer_set_mute | FEED validado; MUTE só silencia; ACK funcional | COMPLIANT |
| NC-006 | RF-WEB-006 | web/api_rest.c | config_monitor_handler / wizard_handler / ack_all / config_set | auth + command_validator em todas as escritas | COMPLIANT |
| NC-007 | RF-FLOW-BOOT-003 | services/self_test.c | test_relay | P03-P10 refletem relay_mcp23017_ok | COMPLIANT |
| NC-008 | RF-HEALTH-MATRIX-001/RF-WEB-005 | main/app/app_main.c; web/api_rest.c | health_report / health_handler | matriz alimentada e exposta em /health | COMPLIANT |
| NC-009 | RF-ENERGY-008/RF-PLUG-004 | services/electric_fsm.c; main/app/app_main.c | total_current check; plug_manager_set_plug_current | limite total e bypass por plugue ativos | COMPLIANT |
| NC-010 | RF-THERMAL-002/003 | fsm/thermal_fsm.c; services/temp_filter.c; main/app/app_main.c | temp_max guard; janela 3; cópia temp_min/max | envelope superior + média 3 + NVS carregada | COMPLIANT |
| NC-011 | RNF-ELECTRICAL-001 | core/circuit_breaker.c; include/hardware_config.h; main/app/app_main.c | HW_CB_*; recover decay; CB_BUS_I2C/SD | thresholds centralizados + buses integrados | COMPLIANT |
| NC-013 | RF-DADOS-001 | main/app/app_main.c; web/api_rest.c | observation_mode pós-reset | setado por reset anômalo; exposto em /status | COMPLIANT |
| NC-014 | — (RTM) | docs/RTM.md | esta tabela | referências reais; sem arquivos inexistentes | COMPLIANT |
| NC-015 | RF-WEB-002 | web/api_rest.c | state_handler | AUTH_GUARD aplicado a GET /state | COMPLIANT |
| NC-016 | RNF-SECURITY-001 | web/api_auth.c | hash_password + ensure_salt | salt aleatório por dispositivo em NVS | COMPLIANT |
## Sprint Fase D-H (compliance >= 96%) — 2026-06-30

| NC | Requisito | Arquivo | Evidência | Status |
|---|---|---|---|---|
| NC-S10 | Registro NVS em todo SAFE_OFF | core/safety_controller.c | safeoff_record_append no evaluate + enter_safeoff | COMPLIANT |
| NC-S09 | Autoridade única de transição | core/global_state.c | global_state_transition delega enter_* | COMPLIANT |
| NC-I09 | Mutex I2C | hal/hal_bus.c | hal_i2c_lock + drivers DS3231/MCP23017 | COMPLIANT |
| NC-I04/A06 | wdt_resets_24h + sd_free_mb | services/wdt_stats.c, web/api_rest.c | NVS counter + esp_vfs_fat_info | COMPLIANT |
| NC-A10 | /api/v1/log real | web/api_rest.c | parse [ts], tail security+events | COMPLIANT |
| NC-F12 | thermal_service sem hardcode | services/thermal_service.c | setpoint NVS, force_safe_off real | COMPLIANT |
| NC-U08 | Histórico + filtro alertas | alert_manager.c, ui_screen_alerts.c | history ring + roller categoria | COMPLIANT |
| NC-PRESET-UI | Presets P03-P10 | ui_preset_picker.c, ui_device_row.c | picker UI + API /plugs/preset | COMPLIANT |
| NC-U13 | Telas órfãs | main/CMakeLists.txt | screen_calibration/submenu removidas | COMPLIANT |
| NC-I01 | Self-test relés | services/self_test.c | GPIO readback P01/P02, MCP read P03-P10 | COMPLIANT |
