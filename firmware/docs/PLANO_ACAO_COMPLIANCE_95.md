# Plano de Ação — Compliance ≥ 95% (Software Only)

| Campo | Valor |
|-------|-------|
| Data | 2026-07-01 (pós-reauditoria) |
| Score atual | **~69%** (`REAUDITORIA_COMPLIANCE_2026-07-01.md`) |
| Meta | **≥ 95%** código + wiring funcional |
| Gap | **~26 pp** |
| Escopo | `docs/COMPLIANCE_SCOPE.md` |

---

## 1. Resultado da reauditoria

- **0 / 14** domínios atingem ≥9.5/10
- **Veredito:** REPROVADO para sign-off software
- **Hardware:** bloqueado até ≥95%

---

## 2. Fases de fechamento (pós-reauditoria)

### Fase R1 — Segurança + ALM (~69% → ~78%)

| NC | Ação | Arquivos |
|----|------|----------|
| NC-S01 | Chamar `safeoff_record_resolve_latest()` na saída SAFE_OFF/EMERGENCY | `app_main.c`, `safeoff_record.c` |
| NC-S02 | Rastrear causa EMERGENCY (ALM-029 vs térmico) na saída | `app_main.c`, `global_state.c` |
| NC-S03 | Implementar resolução por causa em `safeoff_cause_is_resolved()` | `app_main.c` |
| NC-A01 | Clear simétrico para ALMs originados por FSM | `app_main.c`, FSMs |
| NC-A02 | Dono único ALM-013 (remover raise driver/app_main duplicado) | `thermal_fsm.c`, `app_main.c` |
| NC-A02b | Dono único ALM-053 | `electric_fsm.c` ou `alm_monitor.c` |
| NC-A03 | Wire `alert_manager_set_silenced()` na UI/API | `ui_events.c`, `api_rest.c` |

### Fase R2 — Config→runtime + energia (~78% → ~88%)

| NC | Ação | Arquivos |
|----|------|----------|
| NC-C01 | Publicar `EVENT_ID_CONFIG_CHANGED` em todos `config_set_*()` | `config_manager.c` |
| NC-C02 | Completar `app_reload_fsms_from_config()` (min/max, feed, restart) | `app_main.c` |
| NC-E01 | Wh diário por plug com reset à meia-noite | `cdn_energy.c`, `plug_manager.c` |
| NC-E02 | Expor/configurar `max_energy_wh_day` (UI + API + NVS) | `plug_manager.c`, `param_catalog.h` |
| NC-A04 | Wire `ato_fsm_clear_blocked()` (UI manutenção/API) | `ui_events.c`, `api_rest.c` |

### Fase R3 — API + UI operacional (~88% → ≥95%)

| NC | Ação | Arquivos |
|----|------|----------|
| NC-W01 | IP real via `getpeername()` (não proxy headers) | `api_rest.c` |
| NC-W02 | Alinhar códigos erro com `API_ERROR_CODES.md` | `api_rest.c` |
| NC-U01 | Tela térmica editável + `config_set_thermal()` | `ui_screen_config_temperature.c` |
| NC-U02 | Plug toggle na HMI | `ui_events.c`, `ui_device_row.c` |
| NC-U03 | Carrossel: auto-resume após `carousel_pause_s` (não pausa permanente) | `ui_screen_manager.c` |
| NC-U04 | Cabear `ui_preset_picker` nas telas de plugues | `ui_screen_devices_page*.c` |

**Gate:** reauditoria final → cada domínio ≥9.5/10.

---

## 3. Fora do escopo (não bloqueia ≥95%)

- Unity / `test/` / CI de testes
- Flash / smoke / E2E físico
- Montagem de hardware
- TLS / PBKDF2 (melhoria desejável pós-95% se SRS exigir)

---

## 4. Projeção

| Milestone | Score software |
|-----------|----------------|
| Baseline jul/01 | ~51% |
| Pós P0–P4 + refinamentos | ~69% (reauditado) |
| Após Fase R1 | ~78% |
| Após Fase R2 | ~88% |
| Após Fase R3 + sign-off | **≥ 95%** |

*Atualizado 2026-07-01 após reauditoria completa.*
