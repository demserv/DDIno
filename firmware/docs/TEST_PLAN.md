# TEST_PLAN

| Data | 2026-06-27 |
| SRS | v3.11-AF.3 + AF.4 |

| TC | Requisito | Pré-condição | Passos | Resultado esperado | Evidência | Status |
|----|-----------|--------------|--------|-------------------|-----------|--------|
| TC-FSM-TRANS-001 | RF-GLOBAL-002 | Boot OK | Forçar SAFE_OFF→DEGRADED | audit_log + event_bus | log serial | DEFINIDO |
| TC-UI-DISPLAY-001 | RF-UI-DISPLAY-001 | SPI init | ui_display_init | 480×320 ILI9488 | flush callback | DEFINIDO |
| TC-UI-CAROUSEL-001 | RF-UI-CAROUSEL-001 | NORMAL | Aguardar 15s | Avança dashboard→devices→energy | screen_manager | DEFINIDO |
| TC-UI-CAROUSEL-002 | RF-UI-CAROUSEL-001 | SAFE_OFF | tick carrossel | Não avança | carousel_blocked | DEFINIDO |
| TC-UI-VM-001 | RF-GLOBAL-005 | PZEM válido | ui_view_model_update | Dashboard mostra V/A/W reais | g_pzem | DEFINIDO |
| TC-ENERGY-001 | RF-ENERGY-001 | PZEM conectado | pzem_read_all | CRC OK, valid=true | driver_pzem.c | DEFINIDO |
| TC-SAFEOFF-EXIT-001 | RF-GLOBAL-SAFEOFF-EXIT-001 | SAFE_OFF ativo | Resolver causa+ACK+10s | → DEGRADED ou NORMAL | safety_controller | DEFINIDO |
| TC-RELAY-001 | RNF-HW-BOOT-001 | Power-on | relay_init_safe | Todos OFF | GPIO/MCP | DEFINIDO |
| TC-THERMAL-009 | RF-THERMAL-009 | Heater+cooler ON | FSM tick | Ambos OFF + ALM | thermal_fsm | DEFINIDO |
| TC-ATO-OVERFLOW | RF-FSM-ATO-001 | OVERFLOW | ato_fsm_update | SAFE_OFF | ato_fsm | DEFINIDO |
| TC-CMD-001 | RF-COMMAND-* | EMERGENCY | command_validator | REJECT load cmd | command_validator | DEFINIDO |
| TC-API-001 | RF-WEB-001 | Wi-Fi OK | GET /api/v1/state | JSON GlobalState | api_rest | DEFINIDO |
| TC-HEALTH-001 | RF-HEALTH-MATRIX-001 | Subsistema fail | health_report | /health degradado | health_matrix | DEFINIDO |
| TC-WDT-001 | RF-WDT-RECOVERY-001 | Task travada | WDT timeout | Relés OFF pós-reset | wdt_advanced | DEFINIDO |
| TC-SELFTEST-001 | RF-FLOW-SELFTEST-001 | Boot | self_test_run | UI/API resultado | self_test.c | DEFINIDO |
| TC-SPI-001 | RF-HW-SPI-MUTEX-001 | Concurrent SPI | 2 drivers | Sem deadlock, mutex | hal_spi | DEFINIDO |
| TC-I2C-001 | RF-HW-I2C-001 | Barramento OK | scan 0x20/0x68 | Detectados | hal_bus | DEFINIDO |
| TC-STORAGE-001 | RF-STORAGE-001 | SD presente | write log | Arquivo SD | storage_sd | DEFINIDO |
| TC-BOOT-001 | RF-FLOW-BOOT-001 | NVS limpo | Power cycle | Wizard ou NORMAL | app_main | DEFINIDO |

| TC-CFG-ROOT-001 | RF-DATA-CONFIG-ROOT-001 | Config na NVS | config_manager_init | CRC OK, loads 11 grupos | config_root_validate | DEFINIDO |
| TC-CFG-ROOT-002 | RF-DATA-CONFIG-ROOT-001 | NVS corrompida | config_root_load | CRC fail → fallback defaults | config_root_validate=false | DEFINIDO |
| TC-UI-ATO-001 | RF-UI-ATO-001 | Boot OK | ui_screen_manager_show(UI_SCREEN_ATO) | Exibe estado ATO, ADC, bomba | ui_screen_ato_update | DEFINIDO |
| TC-UI-WIZARD-INT-001 | RF-UI-WIZARD-INT-001 | Wizard incompleto | Navegar steps 0-5 | Mostra config thermal/ato/electric real | render_step | DEFINIDO |
| TC-UI-WIZARD-INT-002 | RF-UI-WIZARD-INT-001 | REVIEW confirm | wizard_next_cb step=5 | wizard_completed=true, sistema liberado | g_gs.wizard_completed | DEFINIDO |
| TC-PZEM-MODEL-001 | RF-PZEM-MODEL-001 | Build | grep PZEM-004T | Nenhuma ref a v3.0 | grep PZEM-004T v3.0 → 0 | DEFINIDO |
| TC-AUDIT-SCOPE-001 | RF-AUDIT-SCOPE-001 | Revisao docs | Verificar AUDIT_SCOPE.md | Escopo claro, 3rd party separado | conteudo docs | DEFINIDO |

**Nota**: Execução em bancada ESP32-S3 pendente — testes marcados DEFINIDO aguardam hardware/IDF.
