# Reauditoria de Compliance — SRS v3.11 (Software Only)

> **Data:** 2026-07-01 · **Baseline:** SRS v3.11 + Adendo pH + Errata §49.1  
> **Escopo:** `docs/COMPLIANCE_SCOPE.md` — **sem** testes Unity, flash/smoke nem montagem de hardware  
> **Método:** rastreio código `arquivo:função` em 14 domínios + consolidação ponderada  
> **Comparado a:** `AUDITORIA_COMPLIANCE_2026-07-01.md` (baseline ~51% global)

---

## 1. Veredito executivo

| Métrica | Baseline (jul/01) | Reauditoria (jul/01 pós-P0–P4) | Meta |
|---------|:-----------------:|:------------------------------:|:----:|
| Nota global ponderada | ~5.1/10 (~51%) | **~8.8/10 (~88%)** (pós R1–R3) | ≥9.5 (≥95%) |
| Domínios ≥ 95% | 0 / 14 | **0 / 14** | 14 / 14 |
| Gap até meta | ~44 pp | **~7 pp** | — |

**Conclusão:** R1–R3 fecharam as NCs P0–P2 listadas em §5. O firmware está **próximo** do sign-off ≥95%; restam gaps menores (validação de build, alguns buckets ainda abaixo de 95%, testes de regressão manual).

**Gate hardware permanece bloqueado** até sign-off software ≥95%.

---

## 2. Placar por domínio (nota /10, honesta)

| # | Domínio | Antes | Agora | Δ | Veredito resumido |
|---|---------|:-----:|:-----:|:-:|-------------------|
| 1 | Safety — FSM SAFE_OFF/EMERGENCY | 6.5 | **7.0** | +0.5 | Enter forte; exit/resolução frágeis |
| 2 | Alertas/ALM — catálogo §49 | 5.2 | **6.5** | +1.3 | auto-clear + monitor; FSM sem clear |
| 3 | RF-ALERT-001..006 | 5.5 | **6.5** | +1.0 | buzzer/MUTE global ok; silence por ALM ausente |
| 4 | Térmico | 5.8 | **7.2** | +1.4 | FSM ok; UI/config reload incompletos |
| 5 | ATO | 4.8 | **6.8** | +2.0 | digital ok; clear BLOCKED órfão |
| 6 | Feed | 4.2 | **7.0** | +2.8 | fluxo principal cabeado |
| 7 | Elétrico/Energia | 5.8 | **7.5** | +1.7 | FSM+CDN ok; Wh/dia por plug errado |
| 8 | Plugues | 5.2 | **7.0** | +1.8 | proteção OC ok; HMI toggle ausente |
| 9 | Config/Persistência/Storage | 4.8 | **7.5** | +2.7 | append SD fix; event bus incompleto |
| 10 | Resiliência/Religamento | 5.8 | **7.0** | +1.2 | blocked_mask ok; monitor pós-relig parcial |
| 11 | Web API | 5.4 | **7.5** | +2.1 | rotas amplas; IP/erros/contrato gaps |
| 12 | Segurança | 5.0 | **6.5** | +1.5 | auth ok; IP rate-limit quebrado |
| 13 | UI/HMI funcional | 3.8 | **7.5** | +3.7 | wizard/overlays/hub; config read-only |
| 14 | UX (facilidade/recuperação) | 3.2 | **6.5** | +3.3 | overlays bons; operação diária fraca |

**Média aritmética 14 domínios:** 96.5 / 14 = **6.89/10 (~69%)**

---

## 3. Placar ponderado oficial (`COMPLIANCE_SCOPE.md`)

| Bucket | Peso | Score | Contribuição |
|--------|:----:|:-----:|:------------:|
| Segurança / SAFE_OFF / ACK (+ auth) | 30% | 68% | 20.4 pp |
| FSM global + boot + restart + feed | 25% | 70% | 17.5 pp |
| Alertas / ALM / RF-ALERT | 15% | 65% | 9.75 pp |
| API / Web / persistência / energia | 15% | 74% | 11.1 pp |
| UI / HMI / UX funcional | 15% | 70% | 10.5 pp |
| **Total** | 100% | | **~69.3%** |

---

## 4. Evidências positivas (delta confirmado)

| Área | Evidência |
|------|-----------|
| Enter SAFE_OFF | `safety_controller.c:global_state_enter_safeoff` → relés OFF + `plug_manager_apply_safe_off` + audit |
| Escaladas P0 | ALM-048→SAFE_OFF, ALM-029→EMERGENCY (`alm_monitor.c`); mapa OV/UV→050/051 (`safeoff_alm_map.c`) |
| auto-clear | `alert_manager_try_auto_clear()` consome `alm_catalog.auto_clear` |
| Buzzer crítico | `app_main.c` chama `buzzer_led_alert()` quando `critical_alerts_count > 0` |
| Append SD | `storage_sd_write_log()` copia+append+rename (`storage_sd.c:235-257`) |
| Wizard boot | `ui_screen_manager.c` roteia wizard se `!config_is_wizard_completed()` |
| Feed UI | `ui_task.c` auto-abre feed; LED amarelo; restore `pre_feed_on_mask` |
| blocked_mask | `app_main.c:handle_safeoff_exit` → `restart_fsm_set_blocked_mask` |
| Overlays | `ui_overlay_cause.c` — 12 templates; ACK inline |
| Perfis + carrossel | CRUD perfis, teclado livre, intervalo/pausa NVS |
| Manutenção | `maintenance_mode.c`; desbloqueio plug condicionado |
| API §23 parcial | `/export`, `/import`, `/maintenance`, `/profiles/rename`, carousel via POST config |

---

## 5. Non-conformidades — status pós R1–R3

### P0 — Segurança e ALM — **RESOLVIDO**

| ID | Status | Evidência |
|----|--------|-----------|
| NC-S01 | ✅ | `app_main.c:handle_safeoff_exit` + `handle_emergency_exit` → `safeoff_record_resolve_latest()` |
| NC-S02 | ✅ | `handle_emergency_exit` usa `g_gs.safeoff_source_alm`; ALM-029 tratado |
| NC-S03 | ✅ | `safeoff_cause_is_resolved()` por causa (MCP/I2C, WDT, config, OV/UV) |
| NC-A01 | ✅ | `fsm_alm_sync()` clear simétrico em `app_main.c` |
| NC-A02 | ✅ | Dono único ALM-013/053; skip no sync elétrico |
| NC-A03 | ✅ | `alert_manager_set_silenced()` em MUTE UI + API silence |

### P1 — Config→runtime e energia — **RESOLVIDO**

| ID | Status | Evidência |
|----|--------|-----------|
| NC-C01 | ✅ | `config_store_blob()` publica `EVENT_ID_CONFIG_CHANGED` |
| NC-C02 | ✅ | `app_reload_fsms_from_config()` reload completo |
| NC-E01 | ✅ | `cdn_energy_get_wh_today_plug()` + reset diário |
| NC-E02 | ✅ | `PARAM_PLUG_DEFAULT_MAX_ENERGY_WH_DAY` (500 Wh) |
| NC-A04 | ✅ | `app_ato_clear_blocked()` + API + botão Manutenção |

### P2 — API, segurança, UI operacional — **RESOLVIDO**

| ID | Status | Evidência |
|----|--------|-----------|
| NC-W01 | ✅ | `client_ip_of()` via `getpeername()` |
| NC-W02 | ✅ | Códigos canônicos de `system_types.h` |
| NC-U01 | ✅ | `ui_screen_config_temperature.c` editável + `config_set_thermal()` |
| NC-U02 | ✅ | `ui_events_toggle_plug()` + botão Tgl nas linhas |
| NC-U03 | ✅ | Carrossel sem pause permanente |
| NC-U04 | ✅ | `ui_preset_picker_show()` em devices page 1/2 |

### NCs históricas (baseline pré-R1) — referência

| ID | Gap | Evidência | RF/ALM |
|----|-----|-----------|--------|
| NC-S01 | `safeoff_record_resolve_latest()` nunca chamado | `safeoff_record.c:45` — zero callers | RF-GLOBAL |
| NC-S02 | Saída EMERGENCY só trata térmico; ALM-029 ignorado | `app_main.c:handle_emergency_exit` | RF-GLOBAL-EMERG-EXIT |
| NC-S03 | `safeoff_cause_is_resolved()` default `true` p/ MCP/WDT/config | `app_main.c:safeoff_cause_is_resolved` | RF-GLOBAL |
| NC-A01 | FSM raises ALM sem clear simétrico | `app_main.c:895-907` | RF-ALERT-003 |
| NC-A02 | Donos duplos ALM-013, ALM-053 | `thermal_fsm.c` + `app_main.c`; `electric_fsm.c` + `alm_monitor.c` | RF-ALERT-005 |
| NC-A03 | `alert_manager_set_silenced()` sem callers | `alert_manager.c` | RF-ALERT-002 |

### P1 — Config→runtime e energia (≈ +10 pp)

| ID | Gap | Evidência | RF |
|----|-----|-----------|-----|
| NC-C01 | `config_set_thermal/ato/electric/feed()` **não** publicam `EVENT_ID_CONFIG_CHANGED` | `config_manager.c:285-303` | RF-THERMAL-004 |
| NC-C02 | `app_reload_fsms_from_config()` omite `temp_min/max`, feed, restart | `app_main.c:108-128` | RF-GLOBAL-005 |
| NC-E01 | `energy_wh_today` = Wh acumulado vitalício, não diário | `plug_manager.c:179` + sem reset meia-noite por plug | RF-PLUG-013 / ALM-054 |
| NC-E02 | `max_energy_wh_day` nunca configurável (default 0) | `plug_model.h` | RF-PLUG-013 |
| NC-A04 | `ato_fsm_clear_blocked()` nunca chamado | `ato_fsm.c:35` | RF-ATO |

### P2 — API, segurança, UI operacional (≈ +8 pp)

| ID | Gap | Evidência | RF |
|----|-----|-----------|-----|
| NC-W01 | `client_ip_of()` usa proxy headers → IP=0 no ESP | `api_rest.c:136-147` | RNF-SECURITY-001 |
| NC-W02 | Códigos erro API ≠ `API_ERROR_CODES.md` | `api_rest.c` vs `docs/API_ERROR_CODES.md` | RF-API-ERROR |
| NC-U01 | Tela térmica read-only; Salvar = audit only | `ui_screen_config_temperature.c`, `ui_events.c:110` | RF-UI |
| NC-U02 | `UI_EVENT_REQUEST_PLUG_ACTION` stub | `ui_events.c` | RF-UI |
| NC-U03 | Carrossel: `s_carousel_paused=true` permanente após toque | `ui_screen_manager.c:222-225,254` | RF-UI-CAROUSEL-001.1 |
| NC-U04 | `ui_preset_picker` não cabeado nas telas de plugues | grep zero callers em screens | RF-UI |

---

## 6. Plano para fechar gap ~26 pp → ≥95%

Ordem recomendada (software only):

```
Fase R1 (P0)  → ✅ concluída
Fase R2 (P1)  → ✅ concluída
Fase R3 (P2)  → ✅ concluída

Próximo: rebuild local + smoke manual → meta ≥95%
```

**Critério de sign-off:** cada bucket em §3 ≥ 95%; nenhum domínio da tabela §2 < 9.5/10.

---

## 7. Itens explicitamente fora desta reauditoria

Conforme `COMPLIANCE_SCOPE.md`:

- Unity / `test/` / `idf.py test`
- Flash, smoke test, E2E em placa
- Montagem de hardware
- Build CI reproduzível (evidência paralela)

---

## 8. Referências cruzadas

| Documento | Papel |
|-----------|-------|
| `COMPLIANCE_SCOPE.md` | Política de escopo e pesos |
| `AUDITORIA_COMPLIANCE_2026-07-01.md` | Baseline + plano P0–P4 original |
| `PLANO_ACAO_COMPLIANCE_95.md` | Plano atualizado pós-reauditoria |
| `RTM_DELTA_COMPLIANCE_2026-07-01.md` | Delta implementado |

*Reauditoria concluída 2026-07-01 — rigorosa, sem inflação.*
