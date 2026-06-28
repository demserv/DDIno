# ACTION_PLAN_STATUS

| Data | 2026-06-27 |

| Ação | Status | Arquivos alterados | Evidência | Testes | Pendências |
|------|--------|-------------------|-----------|--------|------------|
| 001 Third-party | IMPLEMENTADO | `concatena.ps1`, `docs/THIRD_PARTY_COMPONENTS.md` | Exclusão managed_components | TC-AUDIT-001 | — |
| 002 ILI9488 | IMPLEMENTADO | `ui/ui_display.c`, `ARCHITECTURE.md`, `pin_map.h` | ILI9488 flush 480×320 | TC-UI-DISPLAY-001 | Build HW |
| 003 GlobalState | IMPLEMENTADO | `core/global_state.c`, `include/global_state.h`, `app_main.c` | bind g_gs, audit+event | TC-FSM-TRANS-001 | — |
| 004 Safety Controller | IMPLEMENTADO | `core/safety_controller.c` (pré-existente) | evaluate+relay_all_off | TC-SAFETY-001 | — |
| 005 PZEM | IMPLEMENTADO | `drivers/driver_pzem.c` (pré-existente) | Modbus+CRC | TC-ENERGY-001 | — |
| 006 Screen Manager | IMPLEMENTADO | `ui_screen_manager.c`, `ui_app.c` | carrossel+blocos | TC-UI-CAROUSEL-001 | — |
| 007 ViewModel | IMPLEMENTADO | `ui_view_model.c` | g_gs+g_pzem+plugs+ALM | TC-UI-VM-001 | — |
| 008 Theme | IMPLEMENTADO | `ui_theme.c`, `ui_theme.h` | 8 estilos LVGL | TC-UI-THEME-001 | — |
| 009 Command Validator | IMPLEMENTADO | `command_validator.c` (pré-existente) | UI+API | TC-CMD-001 | — |
| 010 Fail-safe relés | IMPLEMENTADO | `driver_relay.c`, `relay_abstraction.c` | boot OFF | TC-RELAY-001 | — |
| 011 FreeRTOS tasks | IMPLEMENTADO | `task_manager.c`, `FREERTOS_TASK_MAP.md` | 7 tasks | TC-TASK-001 | — |
| 012 Watchdog | IMPLEMENTADO | `watchdog_guard.c`, `wdt_advanced.c` | heartbeat | TC-WDT-001 | — |
| 013 Health Matrix | IMPLEMENTADO | `health_matrix.c` | 14 subsistemas | TC-HEALTH-001 | — |
| 014 Persistência | IMPLEMENTADO | `config_manager.c`, `storage_sd.c` | NVS+SD+RAM | TC-STORAGE-001 | — |
| 015 Parameter Catalog | IMPLEMENTADO | `docs/PARAMETER_CATALOG.md` | param_catalog.h | TC-CFG-001 | — |
| 016–019 FSMs/Plug | IMPLEMENTADO | `thermal_fsm.c`, `ato_fsm.c`, `electric_fsm.c`, `plug_manager.c` | pré-existente | TC-FSM-* | — |
| 020–022 Feed/Maint/Time | IMPLEMENTADO | `feed_fsm.c`, `global_state.h`, `time_manager.c` | pré-existente | TC-FEED-001 | — |
| 023 API Web | IMPLEMENTADO | `api_rest.c` | 23 handlers | TC-API-001 | — |
| 024 UI operacional | PARCIAL | HMI 12 telas + UI legado | ViewModel real | TC-UI-001 | Telas HMI enriquecer |
| 025 Self-test | IMPLEMENTADO | `self_test.c` | 20 testes HW | TC-SELFTEST-001 | — |
| 026 Alert Manager | IMPLEMENTADO | `alert_manager.c`, `alm_ids.h` | ALM-001..065 | TC-ALM-001 | — |
| 027 Log Manager | IMPLEMENTADO | `storage_sd.c`, `log_ctl.c`, `audit_log.c` | JSONL/SD | TC-LOG-001 | — |
| 028 SPI Mutex | IMPLEMENTADO | `hal_spi.c` | lock/unlock | TC-SPI-001 | — |
| 029 I2C HAL | IMPLEMENTADO | `hal_bus.c` | MCP23017+DS3231 | TC-I2C-001 | — |
| 030 Documentação | IMPLEMENTADO | `docs/*.md` | RTM+relatórios | — | Build final |
