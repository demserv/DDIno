# AUDITORIA DE COMPLIANCE SRS × CÓDIGO — MONITOR AQUÁRIO INTELIGENTE
## Data: 2026-06-30 · Alvo: >= 95% · Idioma: PT-BR

Fontes normativas:
- `SRS TÉCNICO CONSOLIDADO FINAL — MONITOR AQUÁRIO INTELIGENTE.txt`
- `SRS v3.11 COMPLETA — GO CODING READY.txt`

Método: auditoria em 5 domínios por equipe paralela (read-only) + verificação manual
das não-conformidades P0 mais consequentes (evidência confirmada em arquivo:linha).
Regra: zero invenção — itens sem fonte normativa marcados como PENDENTE.

---

## 1. RESULTADO — % DE CONFORMIDADE ATUAL

Pontuação: COMPLIANT=1.0, PARTIAL=0.5, NON-COMPLIANT=0.0, ponderada por criticidade do domínio.

| Domínio | Peso | Compliant | Partial | Non-comp. | Score |
|---|---|---|---|---|---|
| Safety core / relés / estado global / comando | 25% | ~30% | ~45% | ~25% | ~0.53 |
| FSMs (térmica/ATO/elétrica/feed) + boot | 25% | ~40% | ~30% | ~30% | ~0.55 |
| API / auth / rate limit / persistência | 18% | ~30% | ~35% | ~35% | ~0.48 |
| Alertas + UI/HMI | 17% | ~25% | ~40% | ~35% | ~0.45 |
| Infra (self-test/WDT/CB/health/pinagem) | 15% | ~35% | ~40% | ~25% | ~0.55 |

### CONFORMIDADE GLOBAL ESTIMADA: ≈ 52% (faixa 50–58%)

> ATENÇÃO — BLOQUEADOR DE BUILD: `services/command_dispatcher.c` **não compila**
> (símbolos/aridades inválidos). Enquanto não corrigido, `idf.py build` falha e a
> conformidade efetiva executável é **0%**. Ver NC-BUILD-01.

Distância para a meta de 95%: ~43 pontos percentuais. Há base estrutural sólida
(estados globais, camada `relay_abstraction`, FSMs, validator, persistência NVS+CRC,
LVGL oficial), mas persistem não-conformidades safety-critical e funcionais.

---

## 2. NÃO-CONFORMIDADES E PLANO DE AÇÃO DETALHADO

Formato de cada item: **ID | Severidade | Requisito SRS | Arquivo:linha (evidência) |
Problema | Ação detalhada | Critério de aceite**.

---

### 2.0 BLOQUEADOR DE BUILD (P0 — corrigir primeiro)

#### NC-BUILD-01 — `command_dispatcher.c` não compila
- Requisito: build coerente (pré-condição de qualquer conformidade).
- Evidência: `services/command_dispatcher.c:40` chama `plug_manager_set(plug_id, desired_on)` — função **inexistente** (o header expõe `plug_manager_toggle`); `:53` `alert_manager_ack(alert_id)` com **1 argumento** (protótipo exige `(int16_t, uint64_t)`); `:79` `plug_manager_set_mode(plug_id)` com **1 argumento** (protótipo exige `(id, mode)`); `:66` `feed_snapshot_start()` (verificar existência).
- Ação:
  1. `:40` → `return plug_manager_toggle((plug_id_t)plug_id, desired_on);`
  2. `:53` → `return alert_manager_ack(alert_id, (uint64_t)(esp_timer_get_time()/1000000ULL)) ? ESP_OK : ESP_FAIL;` (incluir `esp_timer.h`).
  3. `:79` → o dispatcher não recebe `mode`; alterar assinatura `command_dispatch_set_mode(gs, plug_id, mode)` e repassar, OU remover esta função se não houver caller.
  4. `:66` → confirmar `feed_snapshot_start()` existe em `feed_snapshot.h`; se não, usar o bridge `g_feed_request` ou `feed_fsm_start`.
  5. Se `command_dispatcher` não tiver callers reais (verificar com busca), considerar removê-lo do `CMakeLists.txt` até ser necessário.
- Critério de aceite: `idf.py build` linka sem erro; sem símbolo indefinido; sem warning de aridade.

---

### 2.1 SAFETY CORE / RELÉS / ESTADO GLOBAL / COMANDO

#### NC-S01 — P0 — Anti-flap bloqueia escalonamento crítico
- Requisito: RF-GLOBAL-002, RNF-GLOBAL-ANTIFLAP-001.
- Evidência: `core/safety_controller.c:139-144` — `check_antiflap()` é aplicado a TODAS as transições, inclusive NORMAL→SAFE_OFF/EMERGENCY.
- Problema: um surto de eventos pode "gastar" o orçamento de anti-flap e **impedir** a entrada em SAFE_OFF/EMERGENCY — risco direto à segurança.
- Ação: aplicar anti-flap **apenas** quando `next == NORMAL || next == DEGRADED` (recuperação/rebaixamento). Escalonamento para SAFE_OFF/EMERGENCY deve ser **sempre imediato**. Ex.:
  ```c
  bool is_recovery = (next == SYSTEM_STATE_NORMAL || next == SYSTEM_STATE_DEGRADED);
  if (is_recovery && !check_antiflap(next, now_s * MS_PER_SEC)) { ...return; }
  ```
- Critério de aceite: teste em `test_safety.c` provando que N escalonamentos consecutivos a SAFE_OFF/EMERGENCY nunca são bloqueados; recuperação ainda respeita anti-flap.

#### NC-S02 — P0 — `monitor_only_mode` não bloqueia automação
- Requisito: RF-INSTALL-MONITOR-001 ("Relés permanecem OFF").
- Evidência: `services/plug_manager.c:99-171` (`plug_manager_tick`) não consulta `g_gs.monitor_only_mode`; `drivers/relay_abstraction.c:60-105` (`relay_abstraction_set`) também não.
- Problema: em modo monitor, a automação (térmica/ATO/filtro) continua energizando relés.
- Ação: (a) em `relay_abstraction_set`, negar ON quando `gs.monitor_only_mode` (camada única — solução robusta); (b) em `plug_manager_tick`, forçar `target_on=false` para todos quando `g_gs.monitor_only_mode`. Logar/auditar ativação do modo.
- Critério de aceite: teste com `monitor_only_mode=true` + condições que normalmente ligam relés → nenhum relé liga; comando manual também negado (já coberto pelo validator).

#### NC-S03 — P0 — Mapeamento P01/P02 inconsistente entre módulos
- Requisito: RF-PLUG-002, AF.3 (atribuição canônica de plugues).
- Evidência: `plug_manager.c:49-58` define P01="Bomba ATO"(BOMBA), P02="Aquecedor", P03="Cooler"; `drivers/relay_abstraction.c:21-29` define P01="Aquecedor", P02="Cooler" e trata P01/P02 como críticos por posição; `command_validator.c` trata `plug_id` 1–2 como críticos.
- Problema: três visões divergentes de qual relé físico é aquecedor/cooler/bomba. A exclusão mútua HAL e a dupla confirmação atuam em relés errados; térmica comanda tipos em posições incorretas.
- Ação: definir a atribuição canônica conforme SRS/AF.3 (confirmar no documento qual é P01) e alinhar os TRÊS pontos: `s_default_names`/`s_default_types` do `plug_manager`, `RELAY_NAMES`/`relay_is_critical` do `relay_abstraction` e a regra de criticidade do `command_validator`. Tornar a criticidade derivada do **tipo** (AQUECEDOR/COOLER), não da posição fixa, ou fixar posição única documentada.
- Critério de aceite: um único mapa-fonte; teste verificando que o relé crítico exige confirmação e que a exclusão mútua atua no par aquecedor/cooler reais.

#### NC-S04 — P0 — Bypass morto da abstração (`relay_logical_*`)
- Requisito: RF-PLUG-001 (rota única).
- Evidência: `services/relay_safety_service.c:44-79` — `relay_logical_on/off` chamam `relay_set` direto (sem gate/mutex/self-test/lockout). Sem callers atuais (código morto perigoso).
- Ação: remover `relay_logical_on/off`/`relay_safety_compute` OU reimplementá-las delegando a `relay_abstraction_set`. Garantir por revisão que nenhum módulo chama `relay_set` diretamente, exceto `relay_abstraction`.
- Critério de aceite: busca global confirma `relay_set(` apenas em `relay_abstraction.c`.

#### NC-S05 — P1 — Exclusão mútua não força corte de ambos
- Requisito: RF-THERMAL-009 ("corte imediato de ambos" se heater_on && cooler_on).
- Evidência: `relay_abstraction.c:90-98` apenas nega o 2º ON; `plug_manager.c:205-209` (`set_thermal_request`) aceita ambos `true` sem ação.
- Ação: em `plug_manager_set_thermal_request`, se `heater_on && cooler_on` → solicitar SAFE_OFF (a `thermal_fsm` já gera ALM_060/force_safe_off; garantir propagação) e desligar ambos; nunca energizar nenhum.
- Critério de aceite: TC-THERMAL-MUTEX-001 — injeção heater+cooler → ambos OFF + ALM-060 + SAFE_OFF.

#### NC-S06 — P1 — Dupla confirmação não distingue ON/OFF
- Requisito: RF-PLUG-011 (confirmação para **desligamento manual** de relé crítico) + DEGRADED + ALM-065.
- Evidência: `command_validator.c:15-36` ignora `desired_on`; `relay_abstraction.c:100-104` exige confirm só para ON.
- Ação: alinhar à SRS — definir se a confirmação é para ON, OFF ou ambos (verificar §13.14/§5.10). Implementar o pós-efeito ausente: desligamento manual de P01/P02 → `global_state_enter_degraded` + ALM-065.
- Critério de aceite: teste cobrindo a direção exigida pela SRS + geração de ALM-065/DEGRADED.

#### NC-S07 — P1 — `command_validator_can_start_feed` sem dupla confirmação
- Requisito: RF-FEED-002 / SRS v3.11 (Feed exige confirmação).
- Evidência: `command_validator.c:51-59` não seta `requires_double_confirmation`; `api_rest.c` `/feed` e `ui_events.c` não exigem confirm.
- Ação: setar `requires_double_confirmation=true` em `can_start_feed`; exigir `confirm` no `/api/v1/feed` e diálogo de confirmação na UI antes de emitir `UI_EVENT_REQUEST_FEED_MODE`.
- Critério de aceite: feed sem confirm → negado; com confirm → inicia.

#### NC-S08 — P1 — Tempo mínimo ON/OFF e lockout inativos no caminho principal
- Requisito: RF-PLUG-008.
- Evidência: `plug_manager_toggle` chama `_ex(..., now_ms=0)` (`plug_manager.c`), o que desabilita min ON/OFF; `relay_abstraction.c:32-41` lockout desativado por ausência de `HW_RELAY_LOCKOUT_MS`; `plug_manager_tick` não aplica min ON/OFF.
- Ação: passar `now_ms` real em `plug_manager_toggle`; aplicar min ON/OFF também no tick; definir `HW_RELAY_LOCKOUT_MS` em `hardware_config.h` **se** houver valor normativo (senão PENDENTE).
- Critério de aceite: comandos dentro do tempo mínimo são rejeitados; lockout ativo quando configurado.

#### NC-S09 — P2 — Duas máquinas de estado paralelas
- Evidência: `safety_controller_evaluate` (runtime) vs `global_state_transition` (não chamada).
- Ação: eleger uma única fonte de transição; remover/limitar a outra a testes; unificar anti-flap (hoje há `HW_ANTIFLAP_*` hardcoded em `safety_controller.c` e `config_get_antiflap` em `global_state.c`).
- Critério de aceite: uma só implementação efetiva; parâmetros de anti-flap de uma só fonte.

#### NC-S10 — P2 — `safeoff_record`/`safeoff_alm_map` não integrados
- Evidência: implementados, mas `global_state_enter_safeoff`/`evaluate` não os acionam.
- Ação: registrar histórico de SAFE_OFF e mapear ALM→causa nos enters; expor via API/UI.
- Critério de aceite: cada entrada em SAFE_OFF gera registro persistente com causa/ALM.

---

### 2.2 FSMs + BOOT

#### NC-F01 — P0 — Feed desliga P01–P04 indiscriminadamente
- Requisito: RF-FEED-001 ("desligar só a BOMBA; manter os demais").
- Evidência: `plug_manager.c:113` `else if (feed_active && i < 4) target_on=false;` — desliga índices 0..3 (P01–P04).
- Ação: desligar apenas plugues do tipo bomba/relevantes ao feed conforme SRS (provavelmente só `PLUG_TYPE_BOMBA`), preservando aquecedor/cooler/filtro conforme política.
- Critério de aceite: TC-FEED-001 — em feed, só bomba desliga; demais seguem suas FSMs.

#### NC-F02 — P0 — Cooldown do Feed quebrado
- Requisito: RF-FEED-001 (cooldown).
- Evidência: `feed_fsm.c:52-53` — `feed_fsm_stop` seta `state_started_ms=0`; `:72` calcula `elapsed = now_ms - 0` → cooldown expira instantaneamente.
- Ação: em `feed_fsm_stop`, setar `state_started_ms = now_ms` (passar `now_ms` como parâmetro) para marcar o início do cooldown.
- Critério de aceite: teste — após stop, estado permanece COOLDOWN por `cooldown_s` antes de IDLE.

#### NC-F03 — P0 — Feed não é cancelado em SAFE_OFF/EMERGENCY
- Requisito: RF-FSM-FEED-001.
- Evidência: nenhum `feed_fsm_stop`/`feed_snapshot_clear` ao entrar SAFE_OFF/EMERGENCY em `app_main.c`.
- Ação: ao detectar transição para SAFE_OFF/EMERGENCY, cancelar feed e limpar snapshot.
- Critério de aceite: teste — feed ativo + SAFE_OFF → feed encerra imediatamente.

#### NC-F04 — P0 — `task_sensors_fn` corrompe a temperatura filtrada
- Requisito: RF-THERMAL-002.
- Evidência: `app_main.c` (`task_sensors_fn`) `g_gs.temp_filtered_c = temp_c;` sobrescreve com leitura **bruta** o valor filtrado produzido pelo safety core.
- Ação: remover a sobrescrita; deixar o filtro a cargo do safety_core (uma única fonte de `temp_filtered_c`), ou eliminar a duplicação de leitura entre tasks.
- Critério de aceite: `temp_filtered_c` sempre reflete a média móvel de janela 3.

#### NC-F05 — P0 — Instâncias duplicadas de FSM (thermal/ato/electric_service)
- Requisito: RF-DADOS-001 (fonte única de estado).
- Evidência: `electric_service.c:12-25`, `thermal_service.c`, `ato_service.c` criam FSMs próprias **não atualizadas** no loop; o loop usa `s_*_fsm` em `app_main.c`. Getters/`is_overload` consultam instâncias órfãs → podem "mentir".
- Ação: refatorar os `*_service` para consultarem a instância autoritativa do `app_main` (expor getters em `app_main` ou mover as instâncias para um módulo único). Remover as FSMs duplicadas dos services.
- Critério de aceite: uma única instância por FSM; serviços e API leem o mesmo estado do loop.

#### NC-F06 — P1 — FSMs não propõem DEGRADED (alerta térmico/ATO ERROR/elétrico)
- Requisito: RF-FSM-THERMAL-001, RF-FSM-ATO-001, RF-ENERGY-007/008 (escalonamento).
- Evidência: `app_main.c` fixa `degraded_condition=false`; faixa de alerta térmico, ATO ERROR e estágios elétricos não elevam o estado a DEGRADED.
- Ação: mapear saídas de alerta das FSMs (ALERT/ERROR/zona de alta) para `degraded_condition`; proteções elétricas devem ter dois estágios (DEGRADED → SAFE_OFF) conforme SRS.
- Critério de aceite: testes por FSM provando DEGRADED na zona de alerta e SAFE_OFF na crítica.

#### NC-F07 — P1 — Curto-circuito ignora `tempo_deteccao_curto_ms`; sem bloqueio de plugue
- Requisito: RF-PLUG-014.
- Evidência: `electric_fsm.c:45` usa contagem fixa `>=3`; `tempo_deteccao_curto_ms` não usado; não marca plugue como bloqueado nem desliga plugue isolado.
- Ação: detectar curto por janela temporal `tempo_deteccao_curto_ms` (adicionar timestamps por plugue), marcar `blocked`, desligar o plugue afetado e gerar ALM-055.
- Critério de aceite: TC-PLUG-014 — curto detectado dentro do tempo configurado, plugue isolado e bloqueado.

#### NC-F08 — P1 — Sobrecorrente total usa soma ACS712 (não PZEM)
- Requisito: RF-ENERGY-008 (corrente total do PZEM).
- Evidência: `electric_fsm.c:107-127` soma `plug_currents_a`.
- Ação: usar a corrente total do PZEM como fonte primária da proteção total (ACS712 como complemento por plugue).
- Critério de aceite: proteção total dispara conforme leitura PZEM.

#### NC-F09 — P1 — IDs de ALM elétricos incorretos
- Requisito: RF-ALERT-003 (tabela canônica).
- Evidência: `electric_fsm.c:80,98` usam `ALM_052` para sobre/subtensão (SRS sugere ALM-050/051); PF usa ALM_058 (SRS cita ALM-053).
- Ação: confrontar com a tabela canônica de ALMs da SRS e corrigir os IDs.
- Critério de aceite: cada condição elétrica emite o ALM canônico correto.

#### NC-F10 — P1 — Snapshot de Feed não captura estado real na entrada
- Requisito: RF-PLUG-006/006.1.
- Evidência: `pumps_off_mask = HW_FEED_PUMP_MASK_DEFAULT` (estático); snapshot é periódico, não na ativação.
- Ação: capturar o estado real dos plugues no início do feed; salvar snapshot na entrada; restaurar validando estado global seguro.
- Critério de aceite: queda durante feed → restauração coerente do estado pré-feed.

#### NC-F11 — P1 — Ordem boot: FSMs inicializadas antes do self-test
- Requisito: RF-FLOW-BOOT-001/003.
- Evidência: `app_main.c` init de FSMs (~839) antes do self-test (~903).
- Ação: garantir self-test antes da liberação das FSMs/tasks de controle; manter relés OFF até aprovação.
- Critério de aceite: sequência self-test → liberação; falha crítica mantém SAFE_OFF.

#### NC-F12 — P2 — Stubs/hardcodes em serviços e térmica
- Evidência: `thermal_service.c:16-17` hardcode 25/32/38; `thermal_service.c`/`ato_service.c` `force_safe_off()` são stubs (só log); `electric_service.c:71-72` fallback 127 V; histerese ATO fixa.
- Ação: remover hardcodes (usar NVS/param_catalog); implementar/`force_safe_off` reais ou remover; tornar histerese ATO configurável **se** normativo.
- Critério de aceite: zero hardcode operacional fora de `param_catalog`/`hardware_config`.

#### NC-F13 — P2 — Latch over-temp e trend
- Evidência: `thermal_fsm_clear_over_temp_latch` nunca chamado; trend usa `static` e não é exportado a `g_gs`.
- Ação: ligar reset de latch a um comando manual auditável; expor trend em `g_gs`/API.
- Critério de aceite: latch só sai por ação manual; trend visível.

---

### 2.3 API / AUTH / RATE LIMIT / PERSISTÊNCIA

#### NC-A01 — P0 — `POST /config` persiste só em RAM
- Requisito: RF-WEB-003 / RF-STORAGE-001.
- Evidência: `api_rest.c` `config_set_handler` atualiza `g_gs.monitor_only_mode`/`wizard_completed` sem `config_set_*` (NVS) — diverge de `config_monitor_handler`, que persiste.
- Ação: chamar `config_set_monitor_only`/`config_set_wizard_completed` no `config_set_handler`.
- Critério de aceite: alteração via `/config` sobrevive a reboot.

#### NC-A02 — P0 — Rate limit de login inoperante
- Requisito: RNF-SECURITY-001.
- Evidência: `api_auth.c` `api_auth_login` usa `ip=0` fixo; `login_handler` é isento de `auth_guard`/rate limit; `max_login_attempts`/`login_block_duration_min` não usados; `rate_limit_reset` nunca chamado.
- Ação: extrair IP real no `login_handler` e passar a `api_auth_login`; aplicar `rate_limit_check` ao login; usar parâmetros de config; `rate_limit_reset` após sucesso; auditar bloqueios.
- Critério de aceite: N falhas → bloqueio temporal por IP, com ALM/audit; reset após sucesso.

#### NC-A03 — P0 — Auditoria de segurança ausente nas escritas web
- Requisito: RF-WEB-003 (trilha) / RNF-SECURITY-003.
- Evidência: `api_rest.c` só audita "restart"; login OK/falho, config, wizard, plugs e 401/403 não geram `audit_log_event`. `audit_log.c:35-37` não registra ip/user/result.
- Ação: adicionar `audit_log_event` em login (sucesso/falha), set de config, wizard, comandos de relé e negações; enriquecer a linha de auditoria com ip/user/result.
- Critério de aceite: toda escrita/negação de segurança aparece no canal de auditoria com campos mínimos.

#### NC-A04 — P0 — Wizard/setup escreve config sem validação
- Requisito: RF-WEB-003.
- Evidência: `api_rest.c` `wizard_handler` (POST) grava thermal/ato/electric direto sem `command_validator`; `/auth/password` sem auth por design de setup, mas sem auditoria.
- Ação: validar limites dos parâmetros do wizard (faixas do `param_catalog`); auditar; manter abertura só durante setup (já implementado o gate pós-conclusão).
- Critério de aceite: valores fora de faixa rejeitados; gravações auditadas.

#### NC-A05 — P1 — Schema de `GET /api/v1/state` incompleto
- Requisito: RF-WEB-002 / RF-DADOS-001.
- Evidência: `state_handler` omite ~20 campos canônicos (maintenance_mode, electric_ok, sensores, observation_mode, active_alm_categories, critical_alerts_count, last_reset_reason, config_schema_version, srs_version, time_valid/source, resumos de plugs/sensors/alerts); `firmware_version` hardcoded "3.11".
- Ação: completar o payload conforme a estrutura canônica do GlobalState; usar `g_gs.fw_version`.
- Critério de aceite: TC-WEB-001 — schema bate com a SRS.

#### NC-A06 — P1 — `/api/v1/health` incompleto
- Requisito: RF-WEB-005.
- Evidência: faltam `sd_free_mb`, `wifi_rssi_dbm`, `circuit_breaker_states`, `monitor_only_mode`, `time_valid/source`; `wdt_resets_24h` fixo 0.
- Ação: acrescentar campos; ligar `circuit_breaker_get_state` e contadores reais.
- Critério de aceite: health expõe todos os campos da SRS.

#### NC-A07 — P1 — Export/Import de configuração ausentes
- Requisito: RF-WEB-008, RF-PERSIST-IMPORT-001, RF-STORAGE-004.
- Evidência: `storage_manager.c:180-191` `export/import_from_buffer` → `NOT_SUPPORTED`; sem endpoints `/export` `/import`; backup SD é stub.
- Ação: definir formato canônico de backup (JSON com `config_schema_version` + CRC). Implementar export (snapshot autenticado) e import com **preview + validação CRC + rollback** (reaproveitar `config_staging`/`config_root`). **Se** o formato não estiver normatizado, registrar PENDENTE e especificá-lo antes.
- Critério de aceite: export→import idempotente; import inválido rejeitado sem alterar config ativa.

#### NC-A08 — P1 — Sessão sem expiração por inatividade/renovação
- Requisito: RNF-SECURITY-002.
- Evidência: `api_auth.c` expiração absoluta 1h; `api_auth_validate` não renova; `session_timeout_min` (config) ignorado.
- Ação: renovar `expires_ms` a cada uso (sliding) respeitando `session_timeout_min`.
- Critério de aceite: sessão expira por inatividade configurada; uso renova.

#### NC-A09 — P1 — Escrita atômica/append de log
- Requisito: RF-STORAGE-003/003.1.
- Evidência: `storage_sd.c` não valida integridade antes do rename; append de log reescreve o arquivo inteiro via `.tmp`; promoção de `.tmp` órfão só checa 1º char `{`.
- Ação: validar conteúdo (CRC/parse) antes do rename; usar append real para logs; validar schema/CRC ao promover `.tmp`.
- Critério de aceite: log preserva histórico; `.tmp` órfão só promovido se íntegro.

#### NC-A10 — P2 — CORS / rate genérico / `/api/v1/log` stub
- Evidência: sem headers CORS; rate 30/min vs comentário 5/min; `/log` retorna dados fictícios.
- Ação: aplicar política CORS da SRS; alinhar limites à SRS; implementar `/log` real (a partir do audit/log buffer).
- Critério de aceite: contrato web conforme SRS; `/log` reflete eventos reais.

---

### 2.4 ALERTAS + UI/HMI

#### NC-U01 — P0 — Navegação por keypad inoperante (sem `lv_group_add_obj`)
- Requisito: RF-UI-INPUT-001/002 (operar sem touch).
- Evidência: grupo default criado em `driver_ad_keypad_lvgl.c:80-82`, mas **nenhuma** tela adiciona widgets ao grupo (`lv_group_add_obj` = 0 ocorrências); `ui_focus_apply` nunca usado; tecla FEED (0x80) sem handler.
- Ação: em cada tela, adicionar os objetos focáveis ao `lv_group_get_default()`; aplicar estilo de foco (`ui_focus`); rotear teclas especiais (0x80) para `ui_events_emit` (HOME/FEED/MUTE).
- Critério de aceite: navegação completa por keypad com touch desconectado em todas as telas interativas.

#### NC-U02 — P0 — MUTE sem indicação na UI
- Requisito: RF-UI-MUTE-001 (item 3) / RF-UI-STATUS-001 (item 4).
- Evidência: `buzzer_set_mute` funciona, mas nenhum componente (topbar/footer/VM) exibe MUTE ativo.
- Ação: adicionar campo `mute_active` à view-model (de `buzzer_is_muted()`) e um ícone/badge persistente no topbar; logar expiração do MUTE.
- Critério de aceite: MUTE ativo é visível continuamente; expiração registrada.

#### NC-U03 — P0 — Silence oculta alertas na UI
- Requisito: RF-ALERT-002/005 (crítico permanece visível; silence ≠ resolução).
- Evidência: `alert_manager.c:173-174` (`get_active_slots`) exclui silenciados; `raise_full:51` aborta se silenciado.
- Ação: silence deve suprimir apenas som/repetição, NÃO remover da lista/visual; alerta crítico sempre visível; expor indicação de silence.
- Critério de aceite: TC-ALM-004 — silenciado continua listado e marcado; crítico nunca oculto.

#### NC-U04 — P0 — Wizard de primeiro boot ausente/incompleto
- Requisito: RF-UI-WIZARD-001..005.
- Evidência: boot sempre abre Dashboard (`ui_screen_manager.c:145`); wizard só pelo menu; 6 passos read-only sem validação; faltam etapas (plugues críticos, resiliência, segurança, manutenção, bivolt 127/220).
- Ação: abrir wizard automaticamente quando `!wizard_completed`; implementar as etapas oficiais com editores e validação que bloqueia avanço; reentrada segura (commit transacional).
- Critério de aceite: primeiro boot força wizard; valores inválidos bloqueiam; conclusão persiste.

#### NC-U05 — P0 — Overlays SAFE_OFF/EMERGENCY sem conteúdo normativo
- Requisito: RF-UI-OVERLAY-001.
- Evidência: `ui_critical_overlay.c:56-94` só título/texto fixo — sem ALM ID, severidade, valor, `action_hint` dinâmico.
- Ação: alimentar o overlay com o alerta ativo de maior severidade (ID, severidade, hint) do `alert_manager`.
- Critério de aceite: overlay mostra causa/ALM/hint reais.

#### NC-U06 — P0 — Feed por atalho sem confirmação
- Requisito: RF-UI-SHORTCUT-001 / RF-FEED-002.
- Evidência: `ui_topbar.c:6-10` emite `UI_EVENT_REQUEST_FEED_MODE` direto.
- Ação: exibir `ui_confirm_dialog` antes de emitir o evento de feed.
- Critério de aceite: feed por atalho exige confirmação.

#### NC-U07 — P1 — Tabela canônica de ALMs sem metadados; action_hint não exigido
- Requisito: RF-ALERT-001/003.
- Evidência: `alm_ids.h` só enum; `alert_manager.c` `raise()` default WARNING/hint NULL; ALM-046 sem hint.
- Ação: criar tabela canônica (array) por ID com severity/category/ack_req/auto_clear/action_hint; `raise()` consulta a tabela; validar hint obrigatório p/ HIGH/CRITICAL.
- Critério de aceite: TC-ALM-006 — IDs canônicos consistentes; hint presente em HIGH/CRITICAL.

#### NC-U08 — P1 — Tela de alertas/diagnóstico incompletas
- Requisito: RF-UI-ALERTS-001, RF-UI-DIAG-001/002.
- Evidência: alertas sem categoria/filtro/histórico; diagnósticos sem system_state/heap/sd/wifi/fw; sem drill-down.
- Ação: completar campos e filtros; popular histórico; adicionar drill-down por subsistema.
- Critério de aceite: telas exibem todos os campos da SRS.

#### NC-U09 — P1 — Status bar incompleta + atalhos MUTE/HOME
- Requisito: RF-UI-STATUS-001 / RF-UI-NAV-001 / RF-UI-SHORTCUT-001.
- Evidência: faltam SD, manutenção, self-test falho, feed no footer; `UI_EVENT_NAVIGATE_HOME` não tratado; sem emissor MUTE.
- Ação: completar topbar/footer; tratar HOME→`go_home()`; adicionar atalho MUTE (tecla/ícone).
- Critério de aceite: barra completa; HOME e MUTE funcionam por teclado e touch.

#### NC-U10 — P1 — ACK crítico em massa sem duplo-ACK
- Requisito: RF-ALERT-004.
- Evidência: `ui_events.c` faz ACK em massa simples; existe `alert_manager_ext_ack_critical` (duplo) não usado.
- Ação: para ALM crítico, usar duplo-ACK; ACK simples só p/ não-críticos.
- Critério de aceite: crítico exige confirmação dupla para ACK.

#### NC-U11 — P1 — Carrossel: ordem/pausa
- Requisito: RF-UI-CAROUSEL-001.
- Evidência: ordem sem ATO/Alarmes; pausa usa `hw_alert_pending` em vez de `critical_alerts_count`; sem indicador de pausa.
- Ação: ajustar ordem conforme SRS; pausar por alerta crítico; mostrar indicador de pausa.
- Critério de aceite: carrossel conforme SRS.

#### NC-U12 — P1 — Duração MUTE configurável (5/10/15/até-ACK)
- Requisito: RF-UI-MUTE-001 (Bloco 12/N).
- Evidência: fixa 5 min em `ui_events.c`.
- Ação: adicionar parâmetro de duração em config (UI) com as opções normativas; aplicar no `buzzer_set_mute`.
- Critério de aceite: duração selecionável e registrada.

#### NC-U13 — P2 — Componentes existentes não integrados / stubs de tela
- Evidência: `ui_status_badge`/`ui_focus` não usados; `screen_submenu.c`/`screen_calibration.c` "TODO: port"; config de temperatura read-only; brilho não exposto na HMI.
- Ação: integrar componentes; portar telas pendentes; tornar config editável; expor brilho.
- Critério de aceite: telas funcionais sem stubs.

---

### 2.5 INFRA (SELF-TEST / WDT / CIRCUIT BREAKER / HEALTH / PINAGEM)

#### NC-I01 — P0 — Self-test com stubs (display, touch, P01/P02)
- Requisito: RF-FLOW-SELFTEST-001.
- Evidência: `self_test.c:107-109` (display) e `:201-203` (touch) `r->passed=true`; P01/P02 sempre passam.
- Ação: implementar verificações reais possíveis (ex.: ping ao controlador de display/touch via barramento; readback de GPIO dos relés diretos quando viável). Onde fisicamente impossível verificar, registrar `SKIPPED` (não `PASS`).
- Critério de aceite: nenhum teste safety-critical retorna PASS sem verificação; resultados refletem hardware.

#### NC-I02 — P0 — `CB_BUS_SPI_DISPLAY` órfão + CB não integrado nos drivers
- Requisito: RNF-RESILIENCE-001.
- Evidência: `CB_BUS_SPI_DISPLAY` sem nenhum caller; drivers `mcp23017/ds3231/mcp3208/pzem/ds18b20` não chamam `circuit_breaker_record_*`.
- Ação: instrumentar cada driver (ou o HAL por device) com `record_success/failure` nos pontos de I/O; gatear acessos por `is_available`.
- Critério de aceite: cada barramento abre/fecha conforme falhas reais; matriz reflete estados.

#### NC-I03 — P0 — Health matrix sub-alimentada (8/14) e sem CB
- Requisito: RF-HEALTH-MATRIX-001.
- Evidência: PH, FLOW, BUS_SPI, BUS_I2C, BUS_1WIRE, NVS nunca alimentados; matriz não lê estados do circuit breaker; faltam entradas para self-test/WDT/UI/segurança.
- Ação: alimentar todos os subsistemas aplicáveis (omitir/observar PH/FLOW se não houver sensor físico — confirmar na SRS); mapear `circuit_breaker_get_state`→`health_report(SUB_BUS_*)`; incluir self-test/WDT/UI.
- Critério de aceite: matriz reflete todos os subsistemas reais; sem UNKNOWN indevido.

#### NC-I04 — P1 — WDT per-task timeout não consumido + sem ações de recovery
- Requisito: RF-WDT-001..005 / RF-WDT-RECOVERY-001.
- Evidência: `wdt_advanced.c` usa timeout global; `task_manager.wdt_timeout_ms` nunca lido; `watchdog_guard.c` só loga (sem relés OFF/observation_mode/contador de resets).
- Ação: consumir timeout por task; em falha de heartbeat de task crítica, acionar SAFE_OFF/relés OFF + `observation_mode` + registro de reset reason e contagem.
- Critério de aceite: TC de travamento → relés OFF + observation_mode; timeout por task aplicado.

#### NC-I05 — P1 — CB thresholds não configuráveis (ConfigResilience)
- Requisito: SRS §11.3.5.
- Evidência: `circuit_breaker.c` só usa `HW_CB_*`; ignora `falhas_max_*`, `circuit_breaker_open_duration_s`, `circuit_breaker_enabled`.
- Ação: ligar à config de resiliência (NVS) com fallback aos `HW_CB_*`.
- Critério de aceite: thresholds vêm da config quando presente.

#### NC-I06 — P1 — Self-test sem modelo NOT_RUN/SKIPPED e sem exposição
- Requisito: RF-FLOW-SELFTEST-002/003.
- Evidência: `self_test.h` só `bool passed`; `self_test_get_result` sem callers (API/UI).
- Ação: modelar estado por teste (PASS/FAIL/NOT_RUN/SKIPPED); expor em `/api/v1/health` e na tela de diagnóstico.
- Critério de aceite: resultados por subsistema observáveis em API/UI.

#### NC-I07 — P1 — `ds3231_init` é stub (sem probe)
- Requisito: RF-RTC-* / self-test RTC.
- Evidência: `driver_ds3231.c:24-27` retorna OK sem verificar 0x68.
- Ação: probe I2C ao endereço do RTC antes de declarar OK; refletir em self-test/health.
- Critério de aceite: RTC ausente é detectado.

#### NC-I08 — P2 — Hardcodes residuais (endereço I2C, baud, thresholds)
- Evidência: `driver_mcp23017.c:17`/`self_test.c:86` `0x20`; `hal_bus.c:47`/`driver_pzem.c:14` `9600`; `watchdog_guard.c:13` threshold.
- Ação: substituir por macros canônicas (`HW_I2C_ADDR_MCP23017`, `HW_UART_BAUD`); parametrizar threshold.
- Critério de aceite: zero literais para endereços/baud nos drivers.

#### NC-I09 — P2 — I2C sem mutex
- Evidência: acessos I2C diretos sem mutex (`hal_bus.c`, `driver_mcp23017.c`, `driver_ds3231.c`) — risco sob concorrência (MCP23017 + DS3231 + self-test).
- Ação: introduzir mutex de barramento I2C análogo ao SPI.
- Critério de aceite: acesso I2C serializado.

---

## 3. PENDÊNCIAS SEM FONTE NORMATIVA (não inventar)
- Formato canônico de backup/export (NC-A07): especificar antes de implementar.
- `HW_RELAY_LOCKOUT_MS` (NC-S08): valor não especificado.
- Mapa físico canônico P01..P10 (NC-S03): confirmar atribuição na AF.3 antes de alinhar os módulos.
- Presença física de sensores PH/FLOW (NC-I03): confirmar se aplicáveis ao produto.

---

## 4. ORDEM RECOMENDADA DE EXECUÇÃO PARA ATINGIR >= 95%
1. **NC-BUILD-01** (desbloquear build).
2. **P0 safety**: NC-S01, NC-S02, NC-S03, NC-S04, NC-F01, NC-F02, NC-F03, NC-F04, NC-F05.
3. **P0 API/UI**: NC-A01, NC-A02, NC-A03, NC-A04, NC-U01, NC-U02, NC-U03, NC-U04, NC-U05, NC-U06, NC-I01, NC-I02, NC-I03.
4. **P1** (todas as listadas).
5. **P2** + resolução das pendências normativas.
6. `idf.py build` + suíte Unity + validação por tela (UI/UX) + reauditoria.

Conclusão: conformidade atual ≈ **52%** (e **0% executável** enquanto o build estiver
quebrado). O alvo de 95% exige a execução completa dos planos P0/P1 acima e a
resolução das pendências normativas; só então o fechamento deve ser validado por
build + testes.
