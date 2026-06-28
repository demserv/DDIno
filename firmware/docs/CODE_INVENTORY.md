# CODE_INVENTORY — Monitor Aquário Inteligente

| Campo | Valor |
|-------|-------|
| Data | 2026-06-27 |
| Escopo | `firmware/` excluindo `managed_components/`, `build/` |
| Arquivos produto (.c/.h) | 198 |
| Arquivos third-party | `managed_components/lvgl__lvgl/` (~680 arquivos) |

## Legenda

- **Produto**: código autoral do firmware
- **Third-party**: dependência gerenciada, não alterada manualmente
- **Build**: participa do `main/CMakeLists.txt`

## Módulos de produto

| Módulo | Caminho | Arquivos | Build | Funções principais | @requirement |
|--------|---------|----------|-------|-------------------|--------------|
| Boot | `main/app_main.c` | 1 | Sim | init, loop 50ms, boot sequence | RF-FLOW-BOOT-* |
| Global State | `core/global_state.c`, `include/global_state.h` | 2 | Sim | bind, transition, snapshot, health flags | RF-GLOBAL-* |
| Safety | `core/safety_controller.c` | 2 | Sim | evaluate, exit SAFE_OFF/EMERGENCY | RF-GLOBAL-*, RNF-SAFETY-* |
| Relés | `drivers/driver_relay.c`, `drivers/relay_abstraction.c` | 4 | Sim | relay_all_off, logical on/off | RF-HW-RELAY-* |
| PZEM | `drivers/driver_pzem.c` | 2 | Sim | Modbus RTU, CRC, read_all | RF-ENERGY-001 |
| Display ILI9488 | `ui/ui_display.c` | 2 | Sim | init, lvgl flush, backlight | RF-UI-DISPLAY-001 |
| HMI | `main/ui/hmi/**` | 35 | Sim | screen manager, view model, theme | RF-UI-* |
| UI legado | `ui/**` | 18 | Sim | carrossel legado, telas | RF-UI-CAROUSEL-001 |
| FSM térmica | `fsm/thermal_fsm.c` | 2 | Sim | DS18B20, heater/cooler | RF-THERMAL-* |
| FSM ATO | `fsm/ato_fsm.c` | 2 | Sim | 6 estados, overflow | RF-ATO-* |
| FSM elétrica | `services/electric_fsm.c` | 2 | Sim | proteção, short | RF-ENERGY-* |
| Plug Manager | `services/plug_manager.c` | 2 | Sim | P01–P10, blocked | RF-PLUG-* |
| API REST | `web/api_rest.c` | 4 | Sim | /api/v1/* | RF-WEB-* |
| Command Validator | `services/command_validator.c` | 2 | Sim | validação única UI/API | RF-COMMAND-* |
| Health Matrix | `services/health_matrix.c` | 2 | Sim | subsistemas, snapshot | RF-HEALTH-MATRIX-001 |
| Watchdog | `services/watchdog_guard.c`, `services/wdt_advanced.c` | 4 | Sim | heartbeat, recovery | RF-WDT-* |
| Tasks | `services/task_manager.c` | 2 | Sim | 7 tasks normativas | RF-ARCH-* |
| Config/NVS | `services/config_manager.c` | 2 | Sim | namespaces, antiflap | RF-STORAGE-* |
| Storage SD | `services/storage_sd.c` | 2 | Sim | logs, atomic write | RF-LOG-* |
| Alertas | `services/alert_manager.c` | 2 | Sim | ALM-001..065 | RF-ALERT-* |
| Self-test | `services/self_test.c` | 2 | Sim | boot diagnostics | RF-FLOW-SELFTEST-001 |
| HAL SPI/I2C | `hal/hal_spi.c`, `hal/hal_bus.c` | 4 | Sim | mutex SPI, I2C | RF-HW-SPI-*, RF-HW-I2C-001 |
| Testes | `test/*.c` | 8 | Host/Unity | unitários parciais | TC-* |

## Third-party (excluído de auditoria funcional)

| Componente | Diretório | Modificado | Política |
|------------|-----------|------------|----------|
| LVGL 8.3 | `managed_components/lvgl__lvgl/` | Não | Restaurar via Component Manager |
| ESP-IDF | toolchain | Não | Versão alvo v5.2.2 |

## Scripts de auditoria

| Script | Exclusões aplicadas |
|--------|---------------------|
| `firmware/concatena.ps1` | managed_components, build, .git, .gif/.png/.jpg/.ttf/.bin/.elf/.map |

## Pendências visíveis

| Item | Detalhe |
|------|---------|
| Dual UI | Coexistência `ui/` legado + `main/ui/hmi/` — HMI é caminho preferencial |
| global_state | Corrigido bind para `g_gs` (antes `s_gs` órfão) |
| ViewModel | Corrigido para dados reais (antes parcial/mock) |
