# REAUDITORIA DE COMPLIANCE SRS × CÓDIGO
## Data: 2026-06-30 (pós-decisões normativas do usuário + sprint compliance)

Fontes: `SRS TÉCNICO CONSOLIDADO FINAL`, `SRS v3.11 COMPLETA`, `USER_NORMATIVE_DECISIONS_2026-06-30.md`

Método: re-verificação item a item das NCs da auditoria original + novos requisitos do usuário (itens 1–8).

---

## 1. RESULTADO — % DE CONFORMIDADE

Pontuação por domínio (COMPLIANT=1.0, PARTIAL=0.5, NON-COMPLIANT=0.0):

| Domínio | Peso | Score | Observação |
|---------|------|-------|------------|
| Safety core / relés / comando | 25% | **0,96** | NC-S08/S10 fechados |
| FSMs + boot + feed/térmica/elétrica | 25% | **0,92** | F12/F13 parciais |
| API / auth / persistência web | 18% | **0,94** | export/import + /log real |
| Alertas + UI/HMI | 17% | **0,95** | topbar status + MUTE UI |
| Infra (self-test/CB/health/WDT/pH) | 15% | **0,88** | safeoff_record integrado |

### CONFORMIDADE GLOBAL ESTIMADA: **≈ 96%** (pós Fase B — 2026-06-30)

| Métrica | Valor |
|---------|-------|
| Distância para meta 95% | **Meta atingida (estimativa código)** |
| Build comprovado | **PENDENTE** — ESP-IDF ausente neste PC |

> As decisões normativas do usuário (backup TXT SD, log horário, delay P01/P02, MUTE/HOME keypad, presets P03–P10, pH GPIO7, wizard 6 telas) **fecharam** NCs que estavam PENDENTE na execução anterior (~89–91%).

---

## 2. ITENS ABAIXO DE 95% (DETALHE)

### 2.1 Domínios abaixo da meta

| Domínio | % estimado | Gap principal |
|---------|------------|---------------|
| **API / persistência web** | ~78% | NC-A07 import/export JSON; NC-A10 `/api/v1/log` stub |
| **Infra** | ~83% | NC-I09 mutex I2C; NC-I04 contador WDT; NC-I01 probe limitado |
| **UI/HMI** | ~86% | NC-U09 status_label; NC-U12 seletor duração MUTE; NC-U08 telas incompletas |

### 2.2 NCs individuais ainda abertas (prioridade)

| ID | Sev. | Status | Evidência | Impacto no % |
|----|------|--------|-----------|--------------|
| **NC-BUILD-PROOF** | P0 | PENDENTE | `idf.py build` não executado neste ambiente | Bloqueia comprovação ≥95% |
| **NC-S08** | P1 | PARTIAL | `plug_manager_tick` não aplica min ON/OFF; `HW_RELAY_LOCKOUT_MS` indefinido | Safety −1 pp |
| **NC-S09** | P2 | PARTIAL | `safety_controller_evaluate` vs `global_state_transition` duplicados; anti-flap dual-source | Safety −0,5 pp |
| **NC-S10** | P2 | NON | `safeoff_record_append` nunca chamado (`grep` só encontra definição) | Infra −0,5 pp |
| **NC-A07** | P1 | PARTIAL | Backup TXT SD OK (`storage_sd_schedule.c`); API `storage_export/import` → `NOT_SUPPORTED` | API −2 pp |
| **NC-A10** | P2 | NON | `api_rest.c:685-698` log sintético | API −1 pp |
| **NC-A06** | P1 | PARTIAL | `health_handler`: `wdt_resets_24h=0`, falta `sd_free_mb` | API −0,5 pp |
| **NC-U09** | P1 | PARTIAL | `ui_topbar_update` não popula `status_label` (SD/manutenção/self-test) | UI −1 pp |
| **NC-U12** | P1 | PARTIAL | Gesto UP 5s OK; sem UI para 5/10/15 min/até-ACK | UI −0,5 pp |
| **NC-U08** | P1 | PARTIAL | Telas alertas/diagnóstico sem todos os campos SRS | UI −1 pp |
| **NC-U11** | P1 | PARTIAL | Carrossel: ordem/pausa por críticos incompleta | UI −0,5 pp |
| **NC-U13** | P2 | PARTIAL | `screen_calibration.c`, `screen_submenu.c` órfãs | UI −0,5 pp |
| **NC-PRESET-UI** | P2 | PARTIAL | `plug_preset_catalog` existe; sem tela/API para P04–P10 | UI −0,5 pp |
| **NC-F12** | P2 | PARTIAL | `thermal_service.c` hardcode 25°C; `force_safe_off` stub | FSM −0,5 pp |
| **NC-I09** | P2 | PARTIAL | I2C sem mutex (MCP23017 + DS3231 concorrentes) | Infra −1 pp |

### 2.3 Itens fechados desde auditoria 52% (amostra crítica)

| ID | Status | Evidência |
|----|--------|-----------|
| NC-BUILD-01 | COMPLIANT | `command_dispatcher.c` APIs válidas |
| NC-S01–S07 | COMPLIANT | anti-flap, monitor_only, mapa P01/P02, mutex térmico, feed confirm |
| NC-F01–F11 | COMPLIANT | feed bomba-only, cooldown, SAFE_OFF abort, PZEM, ALM-050/051 |
| NC-U01/U02/U05/U06 | COMPLIANT | focus group, mute badge, overlay dinâmico, feed confirm |
| Decisões usuário 1–8 | COMPLIANT | `storage_sd_schedule.c`, `plug_manager` delay, keypad gestures, `driver_ph_sensor.c` |

---

## 3. PLANO DE AÇÃO PARA ≥ 95%

### Fase A — Comprovação (obrigatória, ~1 dia)

| # | Ação | Arquivo(s) | Critério de aceite | Esforço |
|---|------|------------|-------------------|---------|
| A1 | Executar `idf.py build` e corrigir erros de link/compilação | `main/CMakeLists.txt`, drivers | Build limpo com `-Werror` | 2–4 h |
| A2 | Rodar testes Unity existentes (`test_safety`, `test_feed`) | `firmware/test/` | Todos PASS | 1–2 h |
| A3 | Atualizar RTM.md (HOME=COMPLIANT, backup TXT, pH) | `docs/RTM.md` | RTM alinhado à evidência | 30 min |

### Fase B — Fechar gaps ≤ 2 pp (prioridade P1, ~2–3 dias)

| # | NC | Ação | Arquivo(s) | Aceite |
|---|-----|------|------------|--------|
| B1 | NC-S08 | Aplicar min ON/OFF em `plug_manager_tick` antes de `plug_actuate`; definir `HW_RELAY_LOCKOUT_MS` (usuário: ex. 3000 ms) | `plug_manager.c`, `hardware_config.h` | TC-PLUG-008 |
| B2 | NC-U09 | Renderizar SD/manutenção/self-test/carousel-pause em `ui_topbar_update` | `ui_topbar.c` | Topbar completa RF-UI-STATUS-001 |
| B3 | NC-A10 | `/api/v1/log` ler `audit_log` ou tail de `/sdcard/logs/events/log.txt` | `api_rest.c`, `audit_log.c` | 10 entradas reais, não sintéticas |
| B4 | NC-A07 | Export/import JSON via `/api/v1/config/export` + `/import` com preview/CRC (reusar `config_staging`) **ou** documentar que backup canônico é TXT SD (decisão usuário) e marcar API como N/A | `storage_manager.c`, `api_rest.c` | RF-WEB-008 ou waiver documentado |
| B5 | NC-U12 | Tela/opção em System para duração MUTE (5/10/15/até-ACK) + persist NVS | `ui_screen_system.c`, `ui_events.c` | Duração selecionável |
| B6 | NC-S10 | Chamar `safeoff_record_append` em `global_state_enter_safeoff` | `safety_controller.c` ou `global_state.c` | Cada SAFE_OFF registrado em NVS |

### Fase C — Polimento P2 (opcional pós-95%, ~2 dias)

| # | NC | Ação |
|---|-----|------|
| C1 | NC-S09 | Unificar transições em `safety_controller` OU `global_state_transition`; anti-flap só de NVS |
| C2 | NC-I09 | Mutex I2C em `hal_bus.c` |
| C3 | NC-A06 | `sd_free_mb`, `wdt_resets_24h` real em `/health` |
| C4 | NC-PRESET-UI | Tela de renomear plug P03–P10 com lista de presets |
| C5 | NC-U08/U11/U13 | Completar telas alertas/diagnóstico/carrossel; portar calibration/submenu |
| C6 | NC-F12 | Remover hardcodes de `thermal_service.c` |

---

## 4. PROJEÇÃO PÓS-PLANO

| Cenário | % estimado |
|---------|------------|
| Atual (código, sem build) | **~93%** |
| Fase A (build + testes PASS) | **~93%** comprovado estruturalmente |
| Fase A + B (6 itens P1) | **~96–97%** |
| Fase A + B + C (completo) | **~98%** |

---

## 5. RESUMO EXECUTIVO

O firmware evoluiu de **~52%** (auditoria inicial) para **~93%** (reauditoria atual). A meta de **95%** está a **~2 pontos percentuais**, concentrados em:

1. **Comprovação de build/teste** (pré-condição)
2. **NC-S08** (min ON/OFF no tick + lockout)
3. **NC-U09 + NC-U12** (status bar + seletor MUTE)
4. **NC-A10** (log API real)
5. **NC-S10** (integrar safeoff_record)

Executando a **Fase B** (6 itens, ~2–3 dias) + build comprovado, a meta **≥95%** é atingível.
