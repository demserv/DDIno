# Compliance Report — SRS v3.11-AF.3/AF.4

## 12 PA Items — Implementation Mapping

| PA | Title | Status | Files | Key Functions |
|----|-------|--------|-------|---------------|
| PA-001 | Auth sem hardcode | COMPLETED | `web/api_auth.c`, `web/api_auth.h` | `api_auth_has_password()`, `api_auth_set_password()`, `POST /api/v1/auth/password` |
| PA-002 | API /api/v1 completa | COMPLETED | `web/api_rest.c` | 23 handlers, `plug_set_handler` via plug_manager, calibration GET+POST, feed, wizard, system |
| PA-003 | Command Validator | COMPLETED | `services/command_validator.c`, `.h` | 7 can_* functions, double_confirmation for plugs 1-2 |
| PA-004 | ATO FSM | COMPLETED | `fsm/ato_fsm.c`, `fsm/ato_fsm.h` | 6 states: NORMAL, REFILLING, ERROR, BLOCKED, OVERFLOW, DISABLED |
| PA-005 | Thermal FSM safety | COMPLETED | `fsm/thermal_fsm.c`, `.h` | `over_temp_latched`, `thermal_fsm_clear_over_temp_latch()`, sensor_fail→SAFE_OFF, mutual exclusion |
| PA-006 | SAFE_OFF tracking | COMPLETED | `core/safety_controller.c`, `.h` | `electric_ok`, `safeoff_reason_t` enum, `can_exit_safeoff()`, `can_exit_emergency()` |
| PA-007 | SD atomic/rollback | COMPLETED | `services/storage_sd.c` | `atomic_write_and_rename()`, `.tmp`→rename, fdatasync, rollback |
| PA-008 | Zero hardcode | COMPLETED | `include/hardware_config.h` | All limits via HW_* macros |
| PA-009 | Plug Manager rota | COMPLETED | `services/plug_manager.c`, `.h`, `web/api_rest.c` | `plug_manager_toggle()` verifies safe_off |
| PA-010 | RTOS task map | COMPLETED | `ARCHITECTURE.md` §14 | 6 tasks documented with priority/stack/period |
| PA-011 | hardware_config.h canonical | COMPLETED | `include/hardware_config.h` | HW_PLATFORM_NAME, HW_I2C_*, HW_RELAY_*, HW_ADC_*, HW_TEMP_*, HW_DS3231_*, HW_PZEM_*, HW_NVS_*, HW_WDT_*, HW_TASK_*, HW_SD_*, HW_HTTP_*, HW_UI_*, HW_SENSOR_*, HW_ENERGY_*, HW_ANTIFLAP_*, HW_SAFEOFF_*, HW_EMERGENCY_* |
| PA-012 | UI/HMI overlays | COMPLETED | `ui/ui_screens.c`, `.h` | SAFE_OVERLAY, EMERGENCY_OVERLAY, WIZARD_OVERLAY, MUTE_ICON, `ui_toggle_mute()`, `ui_is_muted()` |

## Build Status
- **Errors**: 0
- **Warnings**: 0 (pre-existing unused `TAG` vars in screen_calibration.c, screen_diagnostic.c)
- **Binary size**: 0xc0d20 bytes (25% free, 0x100000 partition)
- **Toolchain**: xtensa-esp-elf gcc 13.2.0, ESP-IDF v5.3.1

## Known HW Gaps (non-fatal)
1. MCP23017 I2C not present → P03-P10 relays inoperable, critical log emitted
2. SD card not inserted → storage_fs logs timeout, continues without storage
3. WiFi not configured → HTTP API starts on port 80 without network connectivity

## Files Modified (13 total)
`firmware/ARCHITECTURE.md`, `core/safety_controller.c`, `fsm/thermal_fsm.c`, `fsm/thermal_fsm.h`, `include/hardware_config.h`, `services/command_validator.c`, `services/command_validator.h`, `services/storage_sd.c`, `ui/ui_screens.c`, `ui/ui_screens.h`, `web/api_auth.c`, `web/api_auth.h`, `web/api_rest.c`
