# PROMPT MESTRE — FASE L (COMPLIANCE ≥ 95% SEM BUILD)
## PROJETO: MONITOR AQUÁRIO INTELIGENTE — DDINO (ESP32-S3 / ESP-IDF / C / LVGL)
## IDIOMA: PT-BR
## BASELINE: branch `sprint/compliance-95-percent`, commit `d41d065` ou posterior
## AUDITORIA DE ORIGEM: reauditoria rigorosa 2026-06-30 (≈87,5% código, build excluído)

---

## 0. INSTRUÇÃO PARA O AGENTE / DESENVOLVEDOR

Execute **este documento na ordem estrita dos itens L01→L14**.  
**Regra de ouro:** só avance para o item **L(N+1)** quando o item **LN** estiver **COMPLIANT** com evidência verificável no código (grep + leitura de trecho + checklist preenchido).  
Se não atingir a meta: **corrija no mesmo item**; **não** pule, **não** marque COMPLIANT por estimativa.

### 0.1 Proibições absolutas (integridade da nota)

- **PROIBIDO** inventar GPIO, threshold, tempo, ALM, endpoint ou comportamento safety não rastreável à SRS ou ao código existente.
- **PROIBIDO** marcar COMPLIANT sem evidência (`arquivo:linha` ou teste unitário que cubra o comportamento alterado).
- **PROIBIDO** remover safety, bypass de relé ou validação só para “fechar gap”.
- **PROIBIDO** editar `managed_components/lvgl__lvgl`.
- Se dado normativo não existir: registrar **`PENDENTE — DADO NÃO FORNECIDO NO DOCUMENTO FONTE`** e tratar como **PARTIAL** (nunca COMPLIANT).

### 0.2 Build e testes em hardware — BYPASS (fora da nota)

| Item | Tratamento |
|------|------------|
| `idf.py build` | **BYPASS** — será feito em outro PC/oportunidade. **Não entra no cálculo de %.** |
| Execução Unity / HIL | **BYPASS** para nota global. Evidência aceita: **código de teste** que cubra a mudança (revisão estática). |
| Smoke test display/touch físico | **BYPASS** — self-test display/touch permanece SKIPPED aceitável; não inflar nota. |

Coerência estática exigida: sem símbolos óbvios indefinidos, includes corretos, CMakeLists alinhado.

### 0.3 Fórmula de conformidade (sem build)

Pesos fixos por domínio:

| Domínio | Peso |
|---------|------|
| Safety core / relés / transições | 25% |
| FSMs + boot | 25% |
| API / auth / persistência | 18% |
| Alertas + UI/HMI | 17% |
| Infra (CB, health, self-test, RTM) | 15% |

Por NC: **COMPLIANT = 1,0** · **PARTIAL = 0,5** · **NON = 0,0** · **BYPASS = excluído do denominador** (build/test-run).

**Meta desta fase:** ≥ **95%** na fórmula acima **sem** contar build.

### 0.4 Registro de progresso (obrigatório ao fim de cada item)

Criar/atualizar `firmware/docs/FASE_L_PROGRESSO.md` com:

```markdown
## LN — <título>
- Status: COMPLIANT | PARTIAL | NON
- Evidência: `path:linha` (lista)
- Teste estático: `test_*.c` (se aplicável) ou N/A
- Nota parcial domínio: X.XX
- Bloqueio: (vazio ou descrição honesta)
```

---

## 1. ESTADO ATUAL (NÃO REAUDITAR — USAR COMO BASE)

Conformidade estimada **≈87,5%** (build excluído). Itens abertos que esta fase deve fechar:

| ID | Sev. | Domínio | Status atual | Evidência conhecida |
|----|------|---------|--------------|---------------------|
| NC-S09b | P0 | Safety | NON | `app_main.c:241` — `g_gs.system_state = NORMAL` direto |
| NC-S09 / NC-S09c | P1 | Safety | PARTIAL | Anti-flap HW em `safety_controller.c:115-126` vs NVS em `global_state.c:66-82`; `global_state_transition` nunca chamada; duplicação NORMAL em `global_state.c:166-192` |
| NC-CODE-01 | P2 | Safety | PARTIAL | `safety_controller.h:46-47` declaração duplicada |
| NC-A07b | P1 | API | PARTIAL | `config_import_from_json` commit sem sync `g_gs` |
| NC-A07 | P1 | API | PARTIAL | JSON calibration sem `acs712_zero_offset_mv[10]` |
| NC-FEED-01 | P2 | FSM | PARTIAL | `feed_cooldown_min` ausente em `config_manager.c` set_defaults |
| NC-A10b | P2 | API | PARTIAL | Comentário `api_rate_limit.c:1` vs `RATE_LIMIT_MAX_REQ=30` (login usa `api_auth.c` separado) |
| NC-I02 | P2 | Infra | PARTIAL | CB só em subset (`driver_ili9488.c`; loops em `app_main.c`) |
| NC-U13 | P2 | UI | PARTIAL | Órfãos: `screen_submenu.c`, `screen_calibration.c` fora do CMake |
| NC-U14 | P2 | UI | PARTIAL | ACK só em massa (`ui_screen_alerts.c`); sem tap por linha |
| NC-BOOT-01 | P2 | FSM | PARTIAL | Self-test não-crítico → DEGRADED (`app_main.c:1132`) — documentar ou alinhar SRS |
| NC-I04 | P2 | Infra | PARTIAL | `HW_WDT_RESET_MAX_24H` só em `hardware_config.h` |
| RTM | P3 | Infra | PARTIAL | `firmware/docs/RTM.md` desatualizado pós-Fase K |

Itens **já COMPLIANT** (não reabrir salvo regressão): PA-J01 timestamps, NC-S08 min ON/OFF, NC-S10 safeoff_record, RF-ALERT-004 `ack_timeout_s`, duplo-ACK API/UI, import CRC obrigatório, CORS OPTIONS, health 17 subs, I2C mutex, preset picker UI.

---

## 2. ORDEM DE EXECUÇÃO — GATE SEQUENCIAL

```
L01 → L02 → L03 → L04 → L05 → L06 → L07 → L08 → L09 → L10 → L11 → L12 → L13 → L14
         ↑ só avança quando LN = COMPLIANT
```

---

## L01 — NC-S09b: Restart completo sem bypass de `system_state` (P0)

### Problema
Em `firmware/main/app/app_main.c` (~234-249), ramo `restart_fsm_is_complete` atribui `g_gs.system_state = SYSTEM_STATE_NORMAL` diretamente, contornando `global_state_enter_normal()` (audit trail, `electric_ok`, contrato RF-GLOBAL-002).

### Ação
1. Substituir atribuição direta por `global_state_enter_normal(&g_gs, "restart_complete")`.
2. Remover limpezas redundantes já feitas por `enter_normal` (`safeoff_reason`, `safeoff_source_alm`) **somente se** `enter_normal` já as zera (confirmar em `safety_controller.c:94-107`).
3. Manter lógica específica de restart: `restart_in_progress`, `plug_manager_set_restart_mask`, `restart_fsm_abort`, audit específico de restart.
4. Corrigir inconsistência: ramo DEGRADED usa `enter_degraded`; ramo NORMAL deve ser simétrico via `enter_normal` (não escrita direta).
5. **Não** alterar `init_global_state()` linha ~138 (boot frio antes de config) neste item — escopo separado; boot inicial NORMAL com memset é aceitável.

### Critérios de aceite (todos obrigatórios)
- [ ] Grep `g_gs.system_state = SYSTEM_STATE_NORMAL` em `app_main.c` **não** aparece no bloco restart (~234-249).
- [ ] Restart bem-sucedido chama `global_state_enter_normal` com source `"restart_complete"`.
- [ ] `audit_log_state_change` registrado via `enter_normal` (não só `audit_log_event` genérico).
- [ ] Ramo degraded continua usando `global_state_enter_degraded`.

### Evidência mínima
- `app_main.c` diff no bloco restart.
- Grep: zero atribuições diretas a `system_state = NORMAL` **fora** de `init_global_state` e paths já normativos documentados.

### Meta do item
NC-S09b → **COMPLIANT**. Ganho estimado global: **+0,8 pp**.

---

## L02 — NC-CODE-01: Header `safety_controller.h` limpo (P2)

### Problema
Declaração duplicada de `global_state_enter_normal` em `firmware/core/safety_controller.h:46-47`.

### Ação
Remover linha duplicada; manter uma única declaração.

### Critérios de aceite
- [ ] Exatamente **uma** declaração de `global_state_enter_normal` no header.
- [ ] Nenhum outro arquivo duplica a mesma linha consecutiva.

### Meta
NC-CODE-01 → **COMPLIANT**. Ganho: **+0,2 pp** (infra/code hygiene).

---

## L03 — NC-S09c: Anti-flap única fonte (NVS / `config_get_antiflap`) (P1)

### Problema
`safety_controller.c` usa `HW_ANTIFLAP_COOLDOWN_MS`, `HW_ANTIFLAP_WINDOW_MS`, `HW_ANTIFLAP_MAX_TRANSITIONS` (`hardware_config.h:149-151`).  
`global_state.c:66-82` usa corretamente `config_get_antiflap()` → `cooldown_reentrada_s`, `janela_flap_s`, `max_transicoes_flap`.

### Ação (preferida — mínima duplicação)
1. Extrair função compartilhada, ex.: `bool global_state_antiflap_allow(uint64_t now_ms)` em `global_state.c` (+ declaração em `global_state.h`), implementação **idêntica** à lógica NVS já existente em `global_state.c:66-82` (incluindo contadores estáticos — mover ou compartilhar estado de flap de forma thread-safe coerente com mutex existente).
2. Em `safety_controller.c`, substituir `check_antiflap()` local para chamar `global_state_antiflap_allow()`.
3. Remover dependência de `HW_ANTIFLAP_*` no path de transição do safety controller (macros podem permanecer em HW para outros usos se ainda referenciados — grep após mudança).
4. Log antiflap quando bloquear (SRS §3.7 — evidência observável): manter `ESP_LOGW` com parâmetros **da config**, não HW inventado.

### Critérios de aceite
- [ ] `safety_controller.c` **não** referencia `HW_ANTIFLAP_*` no check de recovery.
- [ ] Valores de cooldown/janela/max vêm de `antiflap_params_storage_t` via `config_get_antiflap()`.
- [ ] Comportamento documentado: alterar NVS antiflap reflete no bloqueio (evidência por leitura de código, não runtime).

### Meta
NC-S09c → **COMPLIANT**. Ganho: **+0,4 pp**.

---

## L04 — NC-S09: Unificar `global_state_transition` e `enter_normal` (P1)

### Problema
- `global_state_transition()` definida em `global_state.c:149-197`, **nunca chamada** (grep só definição + header).
- Case `SYSTEM_STATE_NORMAL` duplica lógica de `global_state_enter_normal()` em `safety_controller.c` (sem `event_bus_publish` consistente).

### Ação
1. Refatorar case `SYSTEM_STATE_NORMAL` em `global_state_transition` para:
   - Verificar guardas (EMERGENCY/SAFE_OFF não rebaixam — já existem).
   - Chamar `global_state_antiflap_allow()` antes de NORMAL.
   - Delegar a `global_state_enter_normal(s_gs, source_module)`.
   - Remover bloco duplicado linhas ~166-192.
2. Garantir que `global_state_enter_normal` publique `event_bus_publish(EVENT_ID_NORMAL, NULL)` **se** SRS/contrato exige paridade com transição antiga — se hoje `enter_normal` não publica, adicionar **somente** se já existir padrão equivalente em `enter_safeoff`/`enter_emergency` (verificar antes; não inventar evento novo).
3. **Não** é obrigatório adicionar callers de `global_state_transition` se API pública for mantida para testes futuros — mas documentar em comentário `@requirement RF-GLOBAL-002` que evaluate usa `enter_*` direto e `global_state_transition` é API alternativa equivalente.

### Critérios de aceite
- [ ] Case NORMAL em `global_state_transition` chama `global_state_enter_normal`, sem duplicar assign manual.
- [ ] Zero divergência de campos entre os dois paths NORMAL (grep `system_state = SYSTEM_STATE_NORMAL` restrito a implementações centralizadas).
- [ ] Comentário arquitetural claro: autoridade = `enter_*` + evaluate.

### Meta
NC-S09 → **COMPLIANT** (ou PARTIAL honesto se `event_bus` paridade impossível sem SRS — nesse caso documentar PENDENTE). Ganho alvo: **+0,4 pp**.

---

## L05 — NC-A07b: Sync `g_gs` após import config commit (P1)

### Problema
`config_import_from_json()` (`config_export.c:291-343`) persiste NVS mas **não** atualiza `g_gs`. Campos afetados: `wizard_completed`, `monitor_only_mode`, `maintenance_mode`, `wizard_step`, `config_schema_version` (se aplicável).

### Ação
1. Criar função `void global_state_sync_from_config(global_state_t *gs)` — preferencialmente em `global_state.c` ou reutilizar padrão de `init_global_state()` (`app_main.c:153-162`):
   ```c
   const system_params_storage_t *sys = config_get_system();
   gs->wizard_completed = sys->wizard_completed;
   gs->monitor_only_mode = sys->monitor_only_mode;
   // maintenance_mode se existir em global_state_t
   gs->wizard_step = (wizard_step_t)config_get_wizard_step();
   ```
2. Chamar ao final de import **commit bem-sucedido** (após `config_save_all`, antes do return OK).
3. Chamar também após import via API REST se handler separado existir (grep `config_import_from_json`).
4. Reaplicar calibração runtime mínima: loop `acs712_set_zero_offset(p, cp->acs712_zero_offset_mv[p-1])` espelhando boot (`app_main.c:956-960`) — necessário para coerência pós-import.

### Critérios de aceite
- [ ] Após import commit, `GET /api/v1/state` reflete `monitor_only_mode` / `wizard_completed` importados **sem reboot** (evidência: código sync + campos lidos de `g_gs` em `api_rest.c`).
- [ ] Função sync única, não copy-paste espalhado.
- [ ] Rollback de import **não** chama sync (ou restaura backup coerente).

### Meta
NC-A07b → **COMPLIANT**. Ganho: **+0,5 pp**.

---

## L06 — NC-A07: Export/import JSON — calibração ACS712 completa (P1)

### Problema
`root_to_json` calibration (`config_export.c:113-116`) exporta só `ato_zero_offset_adc` e `temp_offset_c`.  
API `/calibrate` e `param_catalog.h` usam `acs712_zero_offset_mv[10]`.

### Ação
1. Export: adicionar array JSON `acs712_zero_offset_mv` (10 elementos), mesmo formato que `api_rest.c:1080-1082` (`acs712_zero_offsets_mv` ou nome consistente **dentro do config export** — preferir `acs712_zero_offset_mv` alinhado ao struct).
2. Import: parse array com validação de tamanho 10; fallback para valor atual se chave ausente (import parcial sobre base backup — já existente em `json_to_root`).
3. Round-trip: export → import dry_run deve preservar CRC quando nenhum outro campo muda.

### Critérios de aceite
- [ ] Export inclui 10 offsets ACS712.
- [ ] Import parseia 10 offsets e passa em `config_set_calibration`.
- [ ] Import inválido (array tamanho errado) falha validação / rollback.
- [ ] **Não** inventar offsets default novos — usar `ACS712_ZERO_OFFSET_MV` / defaults já em `config_manager.c`.

### Meta
NC-A07 → **COMPLIANT**. Ganho: **+0,3 pp**.

---

## L07 — NC-FEED-01: Default explícito `feed_cooldown_min` (P2)

### Problema
`config_manager.c` set_defaults define `feed_duration_min` mas **não** `feed_cooldown_min` → valor indeterminado em first boot até NVS válida.

### Ação
1. Verificar SRS: **não há** `feed_cooldown_min` normativo nos TXT (RF-FEED-001..003 citam só `feed_duration`).
2. Tratamento **honesto** (escolher UMA opção documentada):
   - **Opção A (recomendada):** `s_feed.feed_cooldown_min = 0` em set_defaults com comentário `@requirement` explicando: cooldown 0 = desabilitado (`feed_fsm.c:81` só entra cooldown se `> 0`). Documentar em RTM (L14).
   - **Opção B:** Se encontrar valor normativo em SRS durante implementação, usar `#define PARAM_FEED_DEFAULT_COOLDOWN_MIN` em `param_catalog.h` **somente** com citação SRS — senão **não usar Opção B**.

### Critérios de aceite
- [ ] `feed_cooldown_min` inicializado explicitamente em set_defaults.
- [ ] RTM ou comentário normativo explica semântica de 0.
- [ ] **PROIBIDO** inventar cooldown “15 min” sem SRS.

### Meta
NC-FEED-01 → **COMPLIANT** (Opção A) ou **PARTIAL** com PENDENTE documentado. Ganho: **+0,1 pp**.

---

## L08 — NC-A10b: Rate limiting — alinhar documentação e implementação (P2)

### Problema
- Login: `api_auth.c:159-179` usa `max_login_attempts` / `login_block_duration_min` da config (default 5) — **correto** para RNF-SECURITY-001 login.
- API geral: `api_rate_limit.h` define `RATE_LIMIT_MAX_REQ 30` / janela 60s; middleware em `api_rest.c:142-183`.
- Comentário linha 1 de `api_rate_limit.c` menciona “5 tentativas/min” — **enganoso** ( mistura login com rate limit geral).

### Ação
1. Corrigir comentário `@requirement` em `api_rate_limit.c` e `api_rate_limit.h` para descrever **rate limit geral de API** (30 req/min/IP ou valor já definido).
2. Remover include morto de `api_rate_limit.h` em `api_auth.c` se não usado (grep `rate_limit_` em api_auth).
3. **Não** alterar `RATE_LIMIT_MAX_REQ` para 5 **a menos que** SRS especifique limite global 5/min (não encontrado — login é separado).

### Critérios de aceite
- [ ] Comentários refletem login (auth) vs API geral (rate_limit) separadamente.
- [ ] Login continua usando `max_login_attempts` da config.
- [ ] Nenhum comentário “5/min” aplicado erroneamente ao middleware geral.

### Meta
NC-A10b → **COMPLIANT**. Ganho: **+0,2 pp**.

---

## L09 — NC-I02: Circuit breaker nos drivers restantes (P2)

### Problema
`driver_ili9488.c` integra CB; drivers `driver_mcp23017.c`, `driver_mcp3208.c`, `driver_ds3231.c`, `driver_ds18b20.c`, `driver_xpt2046.c`, `driver_pzem*.c` (se existir) não registram falhas no barramento correto.

### Ação
1. Mapear barramento por driver (usar enum `cb_bus_id_t` em `circuit_breaker.h`):
   - MCP23017 / DS3231 → `CB_BUS_I2C`
   - MCP3208 / display SPI → `CB_BUS_SPI_ADC` ou `CB_BUS_SPI_DISPLAY` conforme uso real
   - SD → já em `app_main.c`
   - PZEM → `CB_BUS_UART_PZEM`
   - DS18B20 → `CB_BUS_DS18B20`
2. Padrão idêntico a `driver_ili9488.c:29-36`:
   - `if (!circuit_breaker_is_available(bus)) return ERR`
   - success → `circuit_breaker_record_success`
   - failure → `circuit_breaker_record_failure`
3. **Não** duplicar CB onde `app_main.c` já envolve a leitura (ex.: DS18B20 em `read_temp`) — evitar double-count; escolher **uma** camada (driver **ou** app), documentar no progresso.

### Critérios de aceite
- [ ] Cada bus físico tem pelo menos **um** ponto de record_success/failure coerente (driver ou app, não ambos).
- [ ] `/api/v1/health` `circuit_breaker_states` reflete falhas simuláveis por código (grep integração).
- [ ] Nenhum threshold CB inventado — usar `circuit_breaker_configure` defaults HW existentes.

### Meta
NC-I02 → **COMPLIANT** se cobertura completa sem duplicação; senão **PARTIAL** listando buses ainda descobertos. Ganho alvo: **+0,3 pp**.

---

## L10 — NC-U13: Remover ou integrar telas órfãs (P2)

### Problema
Arquivos existem mas **não** estão em `firmware/main/CMakeLists.txt`:
- `firmware/main/ui/hmi/screens/screen_submenu.c` (UI térmica legada; TODO port)
- `firmware/main/ui/hmi/screens/screen_calibration.c` (calibração; fluxo canônico = API `/calibrate`)

### Ação (preferida — mínimo risco)
**Remover** arquivos órfãos se funcionalidade já coberta:
- Térmica: wizard/API/config (confirmar ausência de referências grep `#include.*screen_submenu`).
- Calibração: API REST (`api_rest.c` handler calibrate).

Se referências existirem, integrar ao CMake + screen manager em vez de delete.

### Critérios de aceite
- [ ] Grep zero referências quebradas a arquivos removidos.
- [ ] CMakeLists não precisa incluir telas mortas.
- [ ] Funcionalidade preservada via caminho canônico documentado em L14 RTM.

### Meta
NC-U13 → **COMPLIANT**. Ganho: **+0,15 pp**.

---

## L11 — NC-U14: ACK individual de alerta na UI (P2)

### Problema
`ui_screen_alerts.c` só tem botão ACK ALL. API já suporta ack por id via `alert_manager_ack_with_policy`.

### Ação
1. Em `ui_alert_row.c` / `ui_screen_alerts.c`: adicionar handler click/tecla na linha (keypad-friendly).
2. Chamar mesma política que `ui_events.c` (~72): `alert_manager_ack_with_policy(id, now_s, ALERT_ACK_SOURCE_UI)`.
3. Para críticos com duplo-ACK: reutilizar fluxo existente em `ui_events.c` (não duplicar lógica — extrair helper se necessário, edição mínima).
4. Atualizar VM após ack (`ui_view_model_refresh` ou padrão existente).

### Critérios de aceite
- [ ] Linha de alerta ackável individualmente na tela de alertas.
- [ ] Crítico pendente segundo estágio continua exigindo segundo ack (paridade API).
- [ ] ACK ALL continua funcionando.

### Meta
NC-U14 → **COMPLIANT**. Ganho: **+0,15 pp**.

---

## L12 — NC-BOOT-01: Política boot self-test não-crítico (P2)

### Problema
Falha self-test **não crítica** → DEGRADED (`app_main.c:1132-1142`); falha crítica → SAFE_OFF (correto).

### Ação (documentação — **não** mudar comportamento sem SRS explícita)
1. Localizar requisito SRS sobre self-test boot (ALM-063, RF-SELFTEST ou equivalente).
2. Se SRS **permite** DEGRADED para não-crítico: adicionar seção em `RTM.md` + comentário `@requirement` no bloco 1132 citando SRS.
3. Se SRS **exige** SAFE_OFF: implementar mudança — **somente** com citação linha SRS; senão manter DEGRADED e marcar PARTIAL.

### Critérios de aceite
- [ ] Decisão rastreada a SRS (citação) ou PENDENTE explícito.
- [ ] RTM reflete política escolhida.

### Meta
COMPLIANT se SRS cite DEGRADED; PARTIAL honesto se ambíguo. Ganho: **+0,1 pp**.

---

## L13 — NC-I04: Threshold WDT `HW_WDT_RESET_MAX_24H` (P2)

### Problema
`HW_WDT_RESET_MAX_24H=3` em `hardware_config.h:125` usado em health (`app_main.c:808-811`, `self_test.c:292`) sem param NVS.

### Ação
1. Verificar se catálogo operacional / security / resilience prevê param WDT — se **não**, documentar em RTM como **constante de engenharia** derivada de health degrade (não inventar param novo).
2. Corrigir comentário `@requirement` ligando a campo `wdt_resets_24h` da SRS (health API — observabilidade, não limite normativo).
3. **Opcional** só se param já existir no struct: mover para config. Senão: **não criar** campo novo.

### Critérios de aceite
- [ ] Threshold 3 documentado como engineering default **ou** param NVS se já existir.
- [ ] Health expõe `wdt_resets_24h` real (`wdt_stats.c`) — já COMPLIANT; não regredir.

### Meta
NC-I04 → **COMPLIANT** via documentação. Ganho: **+0,1 pp**.

---

## L14 — RTM + relatório final Fase L (P3)

### Ação
1. Atualizar `firmware/docs/RTM.md`: uma linha por L01-L13 com arquivo, requisito SRS, status COMPLIANT/PARTIAL/NON.
2. Criar `firmware/docs/FASE_L_RELATORIO.md`:
   - Tabela NC × status × evidência
   - **% final por domínio** (fórmula §0.3, build BYPASS)
   - Lista honesta de PARTIAL/NON remanescentes
   - Instruções build para outro PC (referência only, sem impacto na nota)

### Critérios de aceite
- [ ] RTM sem linhas COMPLIANT falsas (cita path real).
- [ ] % global calculada sem build.
- [ ] Se < 95%: lista objetiva do que falta (sem maquiagem).

### Meta
Documentação completa. Ganho: **+0,2 pp** (infra/RTM).

---

## 3. TESTES ESTÁTICOS (BYPASS execução — recomendados por item)

| Item | Teste sugerido | Arquivo |
|------|----------------|---------|
| L01-L04 | TC-FSM-TRANS-001, safeoff exit | `firmware/test/test_safety.c` |
| L05-L06 | Import/export round-trip | novo test ou estender `test_web_state.c` |
| L11 | ACK policy UI | `firmware/test/test_alert.c` |

Adicionar/ajustar testes **só** se cobrirem comportamento novo. COMPLIANT de teste = código de teste revisado; execução PASS = BYPASS.

---

## 4. PROJEÇÃO DE NOTA (HONESTA, SEM BUILD)

| Marco | % estimada |
|-------|------------|
| Baseline (`d41d065`) | ~87,5% |
| Após L01-L04 (Safety) | ~89,5% |
| Após L05-L08 (API/FSM) | ~91,5% |
| Após L09-L14 (Infra/UI/doc) | **~94,5–96%** |

Incerteza ±1 pp se L09 (CB) ou L12 (boot SRS) ficarem PARTIAL.

---

## 5. CHECKLIST FINAL ANTES DE DECLARAR FASE L CONCLUÍDA

- [ ] Todos L01-L14 têm entrada em `FASE_L_PROGRESSO.md`
- [ ] Nenhum item marcado COMPLIANT sem evidência `path:linha`
- [ ] Build **não** usado na nota (registrado como BYPASS externo)
- [ ] Grep final: zero `g_gs.system_state = SYSTEM_STATE_NORMAL` indevido fora init/boot documentado
- [ ] Grep final: zero `HW_ANTIFLAP_*` em `safety_controller.c` recovery path
- [ ] Import JSON round-trip inclui ACS712
- [ ] Órfãos UI removidos ou integrados
- [ ] `FASE_L_RELATORIO.md` com % ≥ 95% **ou** gap list explícito

---

## 6. PROMPT CURTO (COPIAR PARA NOVA SESSÃO AGENTE)

```
Execute firmware/docs/PROMPT_FASE_L_COMPLIANCE.md item a item (L01→L14).

Regras:
- Só avance quando o item atual atingir COMPLIANT com evidência path:linha.
- Não invente dados normativos. PARTIAL honesto se SRS não cobrir.
- Build idf.py = BYPASS (não calcular nota). Testes Unity = BYPASS execução; aceite código de teste.
- Atualize firmware/docs/FASE_L_PROGRESSO.md a cada item.
- Edição mínima; preserve safety; não editar managed_components/lvgl.

Comece em L01 (NC-S09b app_main.c restart path).
Ao terminar L14, gere FASE_L_RELATORIO.md com % por domínio sem build.
```

---

*Documento gerado: 2026-06-30 — Fase L pós-reauditoria rigorosa (commit d41d065).*
