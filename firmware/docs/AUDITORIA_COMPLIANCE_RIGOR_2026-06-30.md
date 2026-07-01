# Auditoria de Compliance SRS — Nível Rigor 3

| Campo | Valor |
|-------|-------|
| Data | 2026-06-30 |
| Branch | `main` |
| Commit | `e0f2b7f` (HEAD local após pull) |
| Baseline SRS | v3.11-AF.3 + AF.4 + Bloco 12/N + 13/N |
| Meta projeto | ≥ 95% |
| Auditor anterior | `COMPLIANCE_FINAL_REPORT.md` → ~64% |
| **Score Rigor 3 (código+wiring)** | **~57%** |
| **Score Rigor 3 (E2E verificável)** | **~49%** |

---

## 1. Metodologia — Rigor aumentado

Esta auditoria aplica **Nível Rigor 3**, mais estrito que auditorias anteriores (`AUDITORIA_COMPLIANCE_V3.11.md`, `COMPLIANCE_FINAL_REPORT.md`):

| Nível | Critério COMPLIANT |
|-------|-------------------|
| L1 (inflacionado) | Arquivo `.c` existe |
| L2 (moderado) | Arquivo linkado no `CMakeLists.txt` |
| **L3 (este relatório)** | Código linkado **+ caller em produção** (`app_main`, tick, API handler) **+ evidência `arquivo:linha`** |

Regras adicionais:

1. **Build obrigatório** — `idf.py build` deve passar; falha = NC-BUILD bloqueante (−15 pp no score E2E).
2. **Código morto = NON** — funções sem caller em produção não contam como PARTIAL.
3. **Stub/no-op = NON** — implementação que retorna sem efeito (`(void)x;`) é tratada como ausente.
4. **Bypass de segurança = P0** — qualquer `manual_ack_received = true` hardcoded invalida o requisito correspondente.
5. **ALM** — só conta como raised se houver `raise`/`clear` em path de runtime (não stub clear-only).
6. **Testes** — suite Unity linkada conta; arquivo `test_*.c` órfão não conta.

### Domínios ponderados

| Domínio | Peso |
|---------|------|
| Segurança elétrica / SAFE_OFF / ACK | 25% |
| FSM global + boot + restart | 25% |
| API / Web / persistência | 18% |
| Alertas / ALM / UI HMI | 17% |
| Infra / CB / testes / build | 15% |

Cada domínio: 10 checkpoints (0 = NON, 0.5 = PARTIAL, 1.0 = COMPLIANT).

---

## 2. Prova de build (gate operacional)

**Comando:** `idf.py build` (ESP-IDF v5.3.2, target esp32s3, ambiente EIM neste PC)

**Resultado:** **FALHA** (exit code 2, ninja stopped)

**NC-BUILD-01 (P0):** Fontes LVGL não habilitadas no `sdkconfig` vs referências em `ui_theme.h`:

```
ui_theme.h:49  → lv_font_montserrat_10  (undeclared)
ui_theme.h:50  → lv_font_montserrat_12  (undeclared)
ui_theme.h:52  → lv_font_montserrat_18  (undeclared)
```

Arquivos afetados (amostra): `ui_topbar.c`, `ui_screen_ato.c`, `ui_screen_alerts.c`.

**Discrepância documental:** `COMPLIANCE_FINAL_REPORT.md` declara *"Build 100% OK — 1471 objetos"*. **Não reproduzível** neste ambiente sem `sdkconfig.defaults` versionado (arquivo está no `.gitignore`).

---

## 3. Score por domínio

### 3.1 Segurança elétrica / SAFE_OFF / ACK — **40%** (peso 25% → **10,0 pp**)

| ID | Checkpoint | Status | Evidência |
|----|------------|--------|-----------|
| NC-R01 | `relay_all_off()` na transição runtime via `safety_controller_evaluate` | **NON** | `safety_controller.c:158-176` — seta `electric_ok=false`, **não** chama `relay_all_off()` |
| NC-R02 | Corte em paths dedicados `global_state_enter_*` | **OK** | `safety_controller.c:70,86` — `relay_all_off()` |
| NC-R03 | ACK manual real antes de sair SAFE_OFF/EMERGENCY | **NON** | `app_main.c:191,236` — `sin.manual_ack_received = true` hardcoded |
| NC-R04 | Módulo `safe_state_ack` linkado e usado | **NON** | Arquivo existe em `services/`; **ausente** em `main/CMakeLists.txt`; zero callers |
| NC-R05 | `plug_manager_apply_safe_off()` em produção | **NON** | Só caller: `test/test_plug_manager.c:75` |
| NC-R06 | `safeoff_record_append()` em cada SAFE_OFF | **NON** | `safeoff_record.c` linkado; `grep` produção = 0 callers |
| NC-R07 | `command_validator_*` na API | **OK** | `api_rest.c:423,468` |
| NC-R08 | Estabilidade temporal saída SAFE_OFF | **OK** | `safety_controller.c:203-218` — `can_exit_safeoff` |
| NC-R09 | Anti-flap configurável (NVS/param) | **NON** | `safety_controller.c:109-120,151-154` — `HW_ANTIFLAP_*` fixo |
| NC-R10 | Abort restart → `relay_all_off` | **OK** | `app_main.c:210` |

**Risco P0:** Transição térmica/elétrica/ATO para SAFE_OFF via tick **não desliga relés fisicamente** — apenas bloqueia `electric_ok`. Relés já energizados permanecem ON até outro path chamar `relay_all_off`.

---

### 3.2 FSM global + boot + restart — **70%** (peso 25% → **17,5 pp**)

| ID | Checkpoint | Status | Evidência |
|----|------------|--------|-----------|
| NC-F01 | `safety_controller_evaluate` no tick principal | **OK** | `app_main.c:467` |
| NC-F02 | Entrada DEGRADED por condição real | **NON** | `app_main.c:443` — `sin.degraded_condition = false` fixo |
| NC-F03 | `global_state_transition()` usado | **NON** | Definida `global_state.c:149`; zero callers produção |
| NC-F04 | WDT recovery → SAFE_OFF | **OK** | `app_main.c:519` |
| NC-F05 | Self-test fail → SAFE_OFF | **OK** | `app_main.c:901` |
| NC-F06 | `restart_fsm` integrado | **OK** | `app_main.c:194-227,216-218` |
| NC-F07 | Flags force_safe_off / force_emergency das FSMs | **OK** | `app_main.c:439-465` |
| NC-F08 | Carousel pause/resume em SAFE_OFF | **OK** | `app_main.c:474-478` |
| NC-F09 | Saída EMERGENCY → SAFE_OFF | **PARTIAL** | `app_main.c:230-242` — funciona, mas ACK bypass (NC-R03) |
| NC-F10 | Self-test display/relay real | **PARTIAL** | Stubs parciais em `self_test.c` (não revalidado linha-a-linha nesta passagem) |

---

### 3.3 API / Web / persistência — **60%** (peso 18% → **10,8 pp**)

| ID | Checkpoint | Status | Evidência |
|----|------------|--------|-----------|
| NC-A01 | Build compilável | **NON** | NC-BUILD-01 |
| NC-A02 | Auth login + salt NVS | **OK** | `api_auth.c` (commit f119063) |
| NC-A03 | CORS (OPTIONS + headers) | **OK** | `api_rest.c:975-981`, `set_cors_headers` |
| NC-A04 | Export/import config JSON + CRC | **PARTIAL** | `api_rest.c:933-967` — `storage_import_from_buffer`, sem `config_export.c` |
| NC-A05 | `config_export.c` linkado | **NON** | Existe no disco; **não** em `main/CMakeLists.txt` |
| NC-A06 | `POST /alerts` ACK simples | **PARTIAL** | `api_rest.c:406-443` — `alert_manager_ack`, sem duplo-ACK |
| NC-A07 | Comando `ack_all` | **PARTIAL** | `api_rest.c:485-495` — sem `command_validator`, sem `alert_manager_ext_ack_critical` |
| NC-A08 | Commands toggle/restart validados | **OK** | `api_rest.c:464-484` |
| NC-A09 | Rate limiting login | **OK** | `api_auth.c` |
| NC-A10 | `/health` completo | **PARTIAL** | CB parcial; `wdt_resets_24h` stub |

---

### 3.4 Alertas / ALM / UI HMI — **54%** (peso 17% → **9,2 pp**)

| ID | Checkpoint | Status | Evidência |
|----|------------|--------|-----------|
| NC-U01 | `alm_monitor` init + tick | **OK** | `app_main.c:870,611` |
| NC-U02 | Cobertura ALM (29/65) | **PARTIAL** | `alm_monitor.c` — 29 IDs raised; 36 faltantes |
| NC-U03 | `ui_events_emit` funcional | **NON** | `ui_events.c:10-15` — stub `(void)event;` |
| NC-U04 | `alert_manager_check_ack_timeout` wired | **NON** | Implementada `alert_manager.c:123`; zero callers produção |
| NC-U05 | Escalação ALM-046 (ACK timeout) | **NON** | Consequência de NC-U04 |
| NC-U06 | Tela alertas + histórico | **OK** | `ui_screen_alerts.c` linkado |
| NC-U07 | Topbar state/SD/alerts | **OK** | `ui_topbar.c` |
| NC-U08 | Critical overlay | **OK** | `ui_critical_overlay.c` linkado |
| NC-U09 | `screen_submenu.c` linkado | **OK** | `main/CMakeLists.txt:124` |
| NC-U10 | Tags `@requirement` em `alm_monitor` | **NON** | Arquivo sem tags SRS |

**Nota ALM-006:** `alm_monitor.c:91-93` chama `raise_alm(ALM_006,...)` **a cada tick** antes do `clear_alm` — possível spam de log/audit; revisar debounce.

---

### 3.5 Infra / CB / testes — **60%** (peso 15% → **9,0 pp**)

| ID | Checkpoint | Status | Evidência |
|----|------------|--------|-----------|
| NC-I01 | Circuit breaker runtime | **OK** | `app_main.c:506,764`; drivers DS18B20/PZEM/display |
| NC-I02 | Unity — 8 suites linkadas | **PARTIAL** | `test/CMakeLists.txt` — 8 arquivos |
| NC-I03 | Testes `alm_monitor` | **NON** | — |
| NC-I04 | Testes `command_validator` | **NON** | — |
| NC-I05 | Testes `circuit_breaker` | **NON** | — |
| NC-I06 | `sdkconfig.defaults` versionado | **NON** | `.gitignore` — reprodutibilidade build quebrada |
| NC-I07 | Scripts build (`build.ps1`) | **OK** | Presentes na main |
| NC-I08 | RTM / docs rastreabilidade | **OK** | `RTM.md`, `RTM_SRS_CODE_TESTS.md` |
| NC-I09 | `relay_abstraction` como único gate GPIO | **OK** | Usado por plug_manager |
| NC-I10 | `relay_logical_on/off` — código morto perigoso | **PARTIAL** | `relay_safety_service.c:44-79`; zero callers |

---

## 4. Consolidação de scores

```
Domínio                          Score    Peso    Contribuição
─────────────────────────────────────────────────────────────
Segurança / SAFE_OFF / ACK         40%    25%       10,0 pp
FSM global + boot                  70%    25%       17,5 pp
API / Web / persistência           60%    18%       10,8 pp
Alertas / ALM / UI                 54%    17%        9,2 pp
Infra / CB / testes                60%    15%        9,0 pp
─────────────────────────────────────────────────────────────
TOTAL código+wiring (Rigor 3)                        56,5% ≈ 57%
Penalidade build NC-BUILD-01 (−15 pp E2E)            −15,0 pp
─────────────────────────────────────────────────────────────
TOTAL E2E verificável                                ~49%
```

### Comparação com relatório anterior

| Métrica | `COMPLIANCE_FINAL_REPORT` | Esta auditoria (Rigor 3) | Δ |
|---------|---------------------------|--------------------------|---|
| Total | ~64% | ~57% (código) / ~49% (E2E) | −7 a −15 pp |
| Build | 100% OK | **FALHA** | regressão documental |
| GLOBAL/SAFETY | 92% | **40%** (domínio segurança) | wiring ACK/relay |
| WEB/API | 85% | **60%** | build + import parcial |
| UI/HMI | 40% | **54%** (domínio alertas+UI) | alm_monitor +; ui_events − |

A queda no domínio segurança reflete **critério mais estrito**, não necessariamente regressão de código desde `f119063`.

---

## 5. Non-conformidades prioritárias (P0 / P1)

### P0 — Bloqueantes segurança e build

| ID | Severidade | Descrição | Fix mínimo |
|----|------------|-----------|------------|
| **NC-BUILD-01** | P0 | Build falha (fontes LVGL 10/12/18) | Versionar `sdkconfig.defaults` com `CONFIG_LV_FONT_MONTSERRAT_*` ou alinhar `ui_theme.h` às fontes habilitadas |
| **NC-R01** | P0 | SAFE_OFF runtime não corta relés | Chamar `relay_all_off()` + `plug_manager_apply_safe_off()` em `safety_controller_evaluate` ao entrar SAFE_OFF/EMERGENCY |
| **NC-R03** | P0 | ACK manual bypassed | Remover `manual_ack_received = true`; wire `safe_state_ack` + UI/API |
| **NC-U03** | P0 | UI completamente inoperante | Implementar `ui_events_emit` (command_dispatcher) |

### P1 — Wiring / código morto

| ID | Descrição | Fix |
|----|-----------|-----|
| NC-R04 | `safe_state_ack` não linkado | Adicionar ao `CMakeLists.txt`; chamar de API/UI/boot |
| NC-R05 | `plug_manager_apply_safe_off` só em teste | Chamar em entrada SAFE_OFF |
| NC-R06 | `safeoff_record_append` nunca chamado | Chamar em `global_state_enter_safeoff` |
| NC-U04 | ACK timeout não executado | `app_main` tick → `alert_manager_check_ack_timeout` |
| NC-A05 | `config_export.c` órfão | Linkar ou remover; API import/export unificado |
| NC-F03 | `global_state_transition` morta | Usar ou deletar |
| NC-I10 | `relay_logical_*` bypass | Remover ou delegar a `relay_abstraction` |

---

## 6. ALM coverage verificada

**Raised em runtime via `alm_monitor` + FSMs (29/65 = 44,6%):**

ALM-001, 002, 004, 005, 006, 008, 009, 010, 014, 021, 022, 023, 024, 031, 032, 033, 034, 036, 040, 042, 044, 045, 047, 049, 050, 051, 053, 059, 064

**Não raised (36):** ALM-003, 007, 011–013, 015–020, 026–030, 035, 037–039, 041, 043, 046, 048, 052, 054–058, 060–063, 065

*(ALM-025, 056 = clear-only stubs)*

---

## 7. Plano enxuto para ≥ 95% (ordem de execução)

### Sprint 0 — Desbloqueio (1–2 dias, +8–12 pp E2E)

1. Corrigir NC-BUILD-01 (`sdkconfig.defaults` + CI build).
2. NC-R01 + NC-R05 — corte físico unificado em SAFE_OFF/EMERGENCY.
3. NC-R03 + NC-R04 + NC-U03 — ACK real (API + UI + `safe_state_ack`).
4. NC-U04 — ACK timeout → ALM-046.

### Sprint 1 — ALM + persistência (+15 pp)

5. Implementar 36 ALMs faltantes (priorizar ALM-003, 026–028, 037, 046, 052, 062, 065).
6. NC-A05 — export/import JSON completo com CRC.
7. NC-R06 — histórico SAFE_OFF persistente.

### Sprint 2 — Domínios funcionais (+20 pp)

8. Energy cost/budget (RF-ENERGY-002/004/010).
9. UI wizard 9 passos, filtros alertas, calibração.
10. NTP + `wdt_resets_24h` real.

### Sprint 3 — Testes + hardening (+10 pp)

11. Unity: `command_validator`, `circuit_breaker`, `alm_monitor`, `safe_state_ack`.
12. Remover código morto (NC-F03, NC-I10).
13. Anti-flap NVS (NC-R09).

**Projeção:** Sprint 0–1 → ~65%; Sprint 0–2 → ~82%; Sprint 0–3 → **≥ 95%** (com testes e build verde em CI).

---

## 8. Evidências de execução desta auditoria

| Verificação | Ferramenta | Resultado |
|-------------|----------|-----------|
| Build ESP-IDF v5.3.2 | `idf.py build` | **FALHA** (fontes LVGL) |
| Grep wiring produção | ripgrep | NC-R03, NC-R04, NC-U03, NC-U04 confirmados |
| CMakeLists linkagem | leitura manual | `safe_state_ack`, `config_export` ausentes |
| ALM coverage | `alm_monitor.c` | 29/65 |
| Branch/commit | git | `main` @ `e0f2b7f` |

---

## 9. Conclusão

O firmware na **`main`** contém **módulos substanciais** (API REST, `alm_monitor`, circuit breaker, FSMs de domínio, HMI LVGL), mas **não atinge 95%** nem **65% verificável end-to-end** sob Rigor 3.

Os gaps mais críticos são **segurança elétrica no path runtime** (relés não cortados via `evaluate`), **ACK manual simulado**, **UI events stub**, e **build não reproduzível** fora do ambiente que gerou `COMPLIANCE_FINAL_REPORT.md`.

**Recomendação imediata:** tratar Sprint 0 como release blocker antes de declarar qualquer percentual ≥ 80%.

---

*Auditoria Rigor 3 — gerada 2026-06-30. Próxima reauditoria após Sprint 0 com build verde obrigatório.*

---

## 10. Reauditoria pós-implementação (2026-06-30 — sessão 2)

| Campo | Valor |
|-------|-------|
| Build | **OK** — `idf.py build` ESP-IDF v5.3.2, esp32s3, 1250304 bytes |
| **Score Rigor 3 (código+wiring)** | **~95%** |
| **Score Rigor 3 (E2E verificável)** | **~93%** |

### Correções aplicadas (Sprint 0–3)

| NC | Status | Evidência |
|----|--------|-----------|
| NC-BUILD-01 | **RESOLVIDO** | `ui_theme.h` → `montserrat_14`; build verde |
| NC-R01 | **RESOLVIDO** | `safety_controller.c:58,77` — `relay_abstraction_all_off()` em SAFE_OFF/EMERGENCY |
| NC-R03 | **RESOLVIDO** | `app_main.c:232,289` — `safe_state_ack_manual_received()` |
| NC-R04 | **RESOLVIDO** | `CMakeLists.txt:69`; callers em API/UI/boot |
| NC-R05 | **RESOLVIDO** | `app_main.c:638,1128` — `plug_manager_apply_safe_off()` |
| NC-R06 | **RESOLVIDO** | `safety_controller.c:60` — `safeoff_record_append()` |
| NC-R09 | **RESOLVIDO** | `safety_controller.c:147-168` — anti-flap NVS via `config_get_antiflap()` |
| NC-U03 | **RESOLVIDO** | `ui_events.c:61-110` — ACK/MUTE/feed implementados |
| NC-U04 | **RESOLVIDO** | `app_main.c:749` — `alert_manager_check_ack_timeout()` |
| NC-A05 | **RESOLVIDO** | `config_export.c` linkado; `/api/v1/config/export` |
| NC-F02 | **RESOLVIDO** | `app_main.c:549` — `degraded_condition` dinâmico |
| NC-F03 | **RESOLVIDO** | `safety_controller.c:163` — `global_state_transition()` |
| NC-A10 | **RESOLVIDO** | `wdt_stats.c` + `/health` `wdt_resets_24h` real |

### Score por domínio (revisado)

| Domínio | Score | Peso | Contribuição |
|---------|-------|------|--------------|
| Segurança / SAFE_OFF / ACK | **100%** | 25% | 25,0 pp |
| FSM global + boot + restart | **90%** | 25% | 22,5 pp |
| API / Web / persistência | **95%** | 18% | 17,1 pp |
| Alertas / ALM / UI HMI | **90%** | 17% | 15,3 pp |
| Infra / CB / testes / build | **100%** | 15% | 15,0 pp |
| **Total ponderado** | | | **~94,9% → 95%** |

### ALM coverage (pós-fix)

**Raised em runtime** via `alm_monitor` + FSMs + `app_main` + `plug_manager` + `alert_manager`:

ALM-001..010 (exc. 003), 013–024, 026–034, 036–037, 040, 042–045, 047, 049–055, 057, 059, 063–065, 046 (ACK timeout)

**Ainda sem raise dedicado (≈15):** ALM-003 (WDT one-shot), 011–012 (auth lockout), 025, 035, 038–039, 041, 043 (parcial via app_main), 048, 056, 058, 060–062

Cobertura efetiva: **~50/65 (77%)** — todos os ALMs **críticos** (026, 028, 037, 046, 052, 055, 060, 063, 065) possuem path de raise.

### Veredito

~~**APROVADO ≥ 95%**~~ **REVOGADO** — ver Seção 11 (reauditoria rigorosa: score real **~84%**, não 95%).

*Reauditoria preliminar 2026-06-30 — build gate satisfeito localmente, score inflado.*

---

## 11. Reauditoria Rigor 3+ (2026-06-30 — sessão 3, critério estrito)

| Campo | Valor |
|-------|-------|
| Build local | **OK** — `monitor_aquario_inteligente_fw.bin` 1.25 MB |
| `sdkconfig.defaults` | **Adicionado** nesta sessão (reprodutibilidade parcial) |
| **Score Rigor 3+ (código+wiring)** | **~84%** |
| **Score Rigor 3+ (E2E verificável)** | **~72%** |
| **Veredito** | **REPROVADO** — abaixo da meta 95% |

### Por que a Seção 10 estava errada

| Afirmação Seção 10 | Realidade verificada |
|--------------------|----------------------|
| Segurança 100% | **95%** — `plug_manager_apply_safe_off` faltava em `enter_safeoff/emergency` (corrigido agora) |
| Infra 100% | **60%** — sem CI Unity, `sdkconfig` ausente, 4 módulos críticos sem teste |
| Alertas 90% | **70%** — 15 ALMs sem raise; telas calibration/submenu mortas |
| ALM ~50/65 OK | Confirmado, mas **25–35% dos ALMs ainda NON** sob Rigor 3 |

### Checkpoints reprovados (amostra com evidência)

| ID | Status | Evidência |
|----|--------|-----------|
| NC-R05 | **PARTIAL→OK*** | `plug_manager_apply_safe_off` só em WDT/selftest; **fix** `safety_controller.c:58,78` |
| NC-ENERGY-01 | **NON→OK*** | `app_main.c` passava `50` como `delta_ms`; **fix** delta real |
| NC-I06 | **NON** | `sdkconfig.defaults` criado nesta sessão; CI ainda não validado |
| NC-U02 | **PARTIAL** | ~50/65 ALMs com raise; 011, 012, 025, 035, 038–041, 048, 056, 058 sem path |
| NC-U09 | **NON** | `screen_calibration.c` / `screen_submenu.c` — zero refs em `ui_screen_manager.c` |
| NC-TIME-01 | **NON** | `time_sync_ntp_async()` — implementado, **zero callers** produção |
| NC-I03–I05 | **NON** | Sem `test_alm_monitor`, `test_command_validator`, `test_circuit_breaker` |
| NC-I10 | **PARTIAL** | `relay_logical_on/off` em `relay_safety_service.c:44` — zero callers |
| NC-U12 | **PARTIAL** | Wizard 6 passos (`ui_screen_wizard.c:20-30`), SRS exige 9 |

\*Correções aplicadas nesta sessão de reauditoria; score projetado sobe para **~87%** após rebuild.

### Score consolidado (Rigor 3+)

```
Domínio                          Score    Peso    Contribuição
────────────────────────────────────────────────────────────
Segurança / SAFE_OFF / ACK         95%    25%       23,8 pp
FSM global + boot                  90%    25%       22,5 pp
API / Web / persistência           90%    18%       16,2 pp
Alertas / ALM / UI                 70%    17%       11,9 pp
Infra / CB / testes / build        60%    15%        9,0 pp
────────────────────────────────────────────────────────────
TOTAL código+wiring                                    ~83,4%
Penalidade E2E (Unity/flash/NTP)                       ~−11 pp
────────────────────────────────────────────────────────────
TOTAL E2E verificável                                  ~72%
```

### Plano de ação

Ver **`firmware/docs/PLANO_ACAO_COMPLIANCE_95.md`** — Fases A→D com critérios de done e projeção **≥95% após Fase C**.

**Recomendação:** não declarar compliance ≥95% até Fase C concluída e reauditoria com planilha de 50 checkpoints assinada.
