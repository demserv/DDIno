# Requirements Traceability Matrix (RTM)

## Version

| Field | Value |
|-------|-------|
| Document Version | 1.0 |
| Date | 2026-06-27 |
| Firmware Version | compliance-fix-v1 |

## Legend

- **ID**: HISTÓRIA identifier (Prompt Mestre 001-025)
- **Title**: Short description
- **Status**: ✅ Complete / ⚠️ Partial / ❌ Missing / 🔲 Out of Scope
- **SRS Reqs**: SRS requirement IDs that map to this História
- **Source Files**: Implementation files
- **Verification**: How compliance is verified

## Traceability Matrix

| ID | Title | Status | SRS Reqs | Source Files | Verification |
|----|-------|--------|----------|--------------|--------------|
| 001 | Refatorar FreeRTOS para Tasks Normativas | ✅ Complete | RF-ARCH-001, RF-ARCH-002, RNF-FREERTOS-001 | services/task_manager.c, include/task_manager.h, main/app_main.c | FREERTOS_TASK_MAP.md |
| 002 | Watchdog por Task e Heartbeat | ✅ Complete | RF-WDT-RECOVERY-001, RF-GLOBAL-004 | services/watchdog_guard.c, include/watchdog_guard.h | FREERTOS_TASK_MAP.md |
| 003 | Mutex SPI Compartilhado | ✅ Complete | RF-HW-SPI-001 | hal/hal_spi.c, include/hal_spi.h | Code review |
| 004 | Display ILI9488 Normativo | ✅ Complete | RF-UI-DISPLAY-001 | include/driver_ili9488.h | Pinagem AF.3 |
| 005 | Bloqueio Crítico Carrossel UI | ✅ Complete | RF-UI-CAROUSEL-001, RF-UI-OVERLAY-001 | ui/ui_screens.c, include/ui_screens.h | Carrossel pausa em SAFE_OFF |
| 006 | Hardcodes → Catálogo de Parâmetros | ✅ Complete | RF-GLOBAL-005, RNF-CONFIG-001, RF-CONFIG-ROOT-001 | include/param_catalog.h, services/config_manager.c | Defaults centralizados |
| 007 | DS18B20 CRC/85°C/Moving Avg | ✅ Complete | RF-THERMAL-001, RF-THERMAL-002 | drivers/driver_ds18b20.c, services/temp_filter.c | CRC + 85°C reject + 3-sample avg |
| 008 | ACS712 RMS por Canal | ✅ Complete | RF-PLUG-003, RFC-PLUG-014, RNF-CALIB-001 | drivers/driver_acs712.c/.h | RMS 20 samples, cal flag degr |
| 009 | ATO Digital ON/OFF | ✅ Complete | RF-ATO-001..005, RF-FSM-ATO-001 | fsm/ato_fsm.c/.h | 6 estados canônicos |
| 010 | Heater/Cooler Exclusão Mútua | ✅ Complete | RF-THERMAL-008, RF-THERMAL-009, RF-FSM-THERMAL-001 | fsm/thermal_fsm.c | both ON → SAFE_OFF + ALM-060 |
| 011 | Config Persistence (NVS) | ✅ Complete | RF-STORAGE-001, RF-STORAGE-005, RF-STORAGE-PARAM-001 | services/config_manager.c, include/config_schema.h | 11 NVS namespaces |
| 012 | SD Log Rotation | ✅ Complete | RF-LOG-001, RF-LOG-RETENTION-001 | services/storage_sd.c | rotate_log_if_needed() |
| 013 | Alert System (ALM) | ✅ Complete | RF-ALERT-001..020, RF-GLOBAL-002 | include/alm_ids.h, services/alert_manager.c | ALM-001..065 lifecycle |
| 014 | Web API REST | ✅ Complete | RF-WEB-001..007, RF-API-001..018 | web/api_rest.c | 23 handlers /api/v1 |
| 015 | UI Alert Bar / Overlay | ✅ Complete | RF-UI-STATUS-001, RF-UI-ALERTS-001 | ui/hmi/components/ui_critical_overlay.c | SAFE_OVERLAY + alert rows |
| 016 | OTA Update | 🔲 Out of Scope | RF-OTA-001 | — | Deferido (pós compliance) |
| 017 | Factory Reset | ✅ Complete | RF-RESET-001..004 | services/reset_handler.c/.h | FSM 6 estados |
| 018 | Self-Test / Diagnostic | ✅ Complete | RF-FLOW-BOOT-001, RF-FLOW-BOOT-003 | services/self_test.c, ui/screens/screen_diagnostic.c | 20 HW tests |
| 019 | Health Check Matrix | ✅ Complete | RF-GLOBAL-003, RF-API-HEALTH-001 | services/health_matrix.c/.h | 14 subsystems + WDT |
| 020 | Email (SMTP) | 🔲 Out of Scope | — | — | Deferido (futuro épico) |
| 021 | RTC (DS3231) | ✅ Complete | RF-TIME-001 | drivers/driver_ds3231.c, services/time_manager.c | I2C + NTP stub |
| 022 | Water Level Sensor | ✅ Complete | RF-ATO-001, RF-ATO-004 | fsm/ato_fsm.c (via ATO) | ATO gerencia nível |
| 023 | pH Sensor | 🔲 Out of Scope | — | — | Deferido (futuro épico) |
| 024 | Sensor Redundancy | 🔲 Out of Scope | — | — | Deferido (futuro épico) |
| 025 | RTM / Compliance Docs | ✅ Complete | RF-RTM-001, RF-RTM-002, RF-RTM-003 | docs/RTM.md, docs/SRS_COMPLIANCE_IMPLEMENTATION_REPORT.md | Este documento |

## Coverage Summary

- Total HISTÓRIAS: 25
- Complete: 21
- Partial: 0
- Out of Scope: 4 (016, 020, 023, 024)
- Coverage (in scope): 100% (21/21)
- Coverage (total): 84% (21/25)

## Compliance Sign-off

- **Architecture Review**: [ ]
- **Safety Review**: [ ]
- **Code Review**: [ ]
- **Integration Test**: [ ]
- **Final Approval**: [ ]
