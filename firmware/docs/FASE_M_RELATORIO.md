# Fase M — Correções pós-reauditoria (≥95% código, build BYPASS)

**Data:** 2026-06-30

## Alterações

| ID | Arquivo | Evidência |
|----|---------|-----------|
| NC-S08b | `plug_manager.c/h` | `plug_manager_reload_limits()` ← `config_get_plug_limits()`; chamado em init + `global_state_sync_from_config` |
| NC-U13 | `screens/screen_submenu.c`, `screen_calibration.c` | Removidos do tree |
| NC-U14b | `ui_app.c/h`, `ui_events.c` | `ui_app_refresh_now()` após ACK unitário e ACK ALL |
| NC-S09-API | `safety_controller.c`, `global_state.c` | `evaluate` → `global_state_transition()` |
| NC-A07c | `global_state.c` | `config_schema_version` sincronizado no import |
| NC-I02b | `circuit_breaker.c`, RTM | Thresholds HW documentados (sem param NVS inventado) |

## Conformidade estimada (sem build)

≈ **95,5%** — ver FASE_L_RELATORIO fórmula com domínios atualizados.

## BYPASS

- `idf.py build` — pendente outro PC
- Unity / HIL — pendente
