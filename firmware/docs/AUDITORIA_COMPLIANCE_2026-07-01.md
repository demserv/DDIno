# Auditoria de Compliance vs SRS v3.11 — Honesta e Exaustiva

> **Data:** 2026-07-01 · **Baseline:** SRS v3.11 + Adendo pH v3.11 + Errata §49.1
> **Escopo:** APENAS software/funcional (código + wiring em runtime). Política completa:
> `docs/COMPLIANCE_SCOPE.md`.
> **Excluídos do cálculo de ≥ 95%:** montagem de hardware, flash/smoke/E2E físico,
> suíte Unity/`test/`, CI de testes e build reproduzível (evidência paralela, não penaliza score).
> **Gate:** montagem de hardware **somente após** sign-off de software ≥ 95% SRS.
> **Método:** leitura do texto normativo da SRS + rastreio de evidência (arquivo:função)
> em 6 auditorias de domínio independentes + verificação direta de alegações de alto impacto.
> **Postura:** rigorosa e sem inflar. As marcações "COMPLETED" em `COMPLIANCE_REPORT.md` e
> nas RTMs internas **superestimam** o que está de fato cabeado e funcional em runtime.

---

## 1. Placar de compliance (nota 0–10, honesta)

> Meta contratada: **≥ 9.5** por domínio (≥ 95% global). **Reauditoria 2026-07-01:** ver
> `docs/REAUDITORIA_COMPLIANCE_2026-07-01.md` — **~6.9/10 (~69%)** global ponderado;
> **0/14 domínios ≥ 95%**.

| # | Domínio | Baseline | Reaudit. | Veredito |
|---|---------|:--------:|:--------:|----------|
| 1 | Safety — FSM SAFE_OFF/EMERGENCY | 6.5 | **7.0** | Enter forte; exit/resolução frágeis |
| 2 | Alertas/ALM — catálogo §49 | 5.2 | **6.5** | auto-clear ok; FSM sem clear simétrico |
| 3 | RF-ALERT-001..006 | 5.5 | **6.5** | buzzer ok; silence por ALM ausente |
| 4 | Térmico | 5.8 | **7.2** | FSM ok; UI/config reload incompletos |
| 5 | ATO | 4.8 | **6.8** | digital ok; clear BLOCKED órfão |
| 6 | Feed | 4.2 | **7.0** | fluxo principal cabeado |
| 7 | Elétrico/Energia | 5.8 | **7.5** | FSM ok; Wh/dia por plug errado |
| 8 | Plugues | 5.2 | **7.0** | OC/bypass ok; HMI toggle ausente |
| 9 | Config/Persistência/Storage | 4.8 | **7.5** | append fix; event bus incompleto |
| 10 | Resiliência/Religamento | 5.8 | **7.0** | blocked_mask ok; monitor parcial |
| 11 | Web API | 5.4 | **7.5** | rotas amplas; IP/contrato gaps |
| 12 | Segurança | 5.0 | **6.5** | auth ok; rate-limit IP quebrado |
| 13 | UI/HMI funcional | 3.8 | **7.5** | wizard/overlays; config read-only |
| 14 | UX | 3.2 | **6.5** | overlays bons; operação diária fraca |

### Nota global (média ponderada — escopo software)
> **≈ 6.9 / 10 (≈ 69%)** — subiu de ~51%; **~26 pp abaixo** da meta de 95%.
> Detalhes: `REAUDITORIA_COMPLIANCE_2026-07-01.md`.

---

## 2. Achados transversais críticos (afetam várias notas)

### 2.1 Duplicação de código/lógica (viola "sem funções duplicadas")
| Duplicação | Locais | Ação |
|------------|--------|------|
| Subsistema de alertas paralelo **morto** | `services/alm_ctl.c` vs `alert_manager.c` | Remover `alm_ctl.c` (sem callers de produção) |
| ALM elétricos com **dono duplo** | `electric_fsm.c` (raise) + `alm_monitor.c` (raise) p/ 022/050/051/053 | Escolher 1 dono (recomendo `alm_monitor`) |
| ALM-013 com **3 donos** | `driver_ds18b20.c` + `thermal_fsm.c` + `app_main.c` (severidades divergentes) | Único dono; remover raise do driver |
| ALM-049 com **2 semânticas** | pH + tensão no mesmo ID em `alm_monitor.c` | Separar; tensão usa ID próprio |
| FSMs órfãs (falsa sensação de completude) | `thermal_service.c`, `ato_service.c` (25/32/38 hardcoded, nunca ticadas) | Remover ou passar a delegar à instância viva |
| Drivers de entrada duplicados **fora do build** | `drivers/ui_keypad.c`, `drivers/ui_touch.c` (não no CMake) | Remover (código morto, como os órfãos de tela já removidos) |
| Rate-limit duplicado | `api_auth.c` (fail counter) + `api_rate_limit.c` | Unificar numa política |
| Geração de token duplicada | `api_auth.c` + `sec_policy.c` (este último morto) | Remover trilha morta |
| 3 buffers RAM de log sobrepostos | `storage_sd.c` (64), `storage_atomic.c` (256), `storage_facade.c` (128) | Consolidar num ring com prioridade |
| Componentes UI compilados **sem callers** | `ui_confirm_dialog.c`, `ui_preset_picker.c`, `ui_inline_hint.c`, `ui_focus.c`, `ui_screen_diag_detail.c` | Cabear ou remover |

### 2.2 Ciclo de vida de ALM incompleto (afeta notas 2/3)
- `alm_catalog.auto_clear` é **armazenado mas nunca consumido** — não há scheduler de
  auto-clear em `alert_manager.c`. Dezenas de ALMs (001/002/010/015/016/047/049…) nunca limpam.
- Vários `raise` deixam **mensagem vazia** (SRS exige mensagem + ação recomendada).
- `alert_manager_set_silenced()` **nunca é chamado**; `buzzer_led_alert()` **sem callers** →
  alertas críticos não acionam o buzzer automaticamente (RF-ALERT-002/006).

### 2.3 Escaladas de segurança ausentes (afeta nota 1)
- **ALM-048 / falha MCP23017** não força SAFE_OFF (relés comprometidos ficam sem proteção).
- **ALM-029** (excesso de resets) não escala para EMERGENCY (SRS pede EMERGENCY).
- **ALM-020** (ATO) nunca é levantado (bloqueado por `force_safe_off`).
- Mapa causa→ALM erra OV/UV (mapeia ALM-052 em vez de 050/051) em `safeoff_alm_map.c`.
- Saída de EMERGENCY usa causa/ALM errados (`app_main.c` — `FSM_INVALID`/`ALM-003`).

### 2.4 Config não afeta runtime (afeta notas 4/5/7)
- `config_set_thermal()/ato()/electric()` persistem em NVS mas **não** chamam
  `*_fsm_set_config()` na instância viva → mudanças só valem após reboot.

### 2.5 UI não cabeada (afeta notas 13/14)
- Wizard **não roteado no boot** (`ui_screen_manager.c:149` mostra DASHBOARD incondicional).
- `UI_EVENT_NAVIGATE_HOME/BACK`, `REQUEST_SAVE_THERMAL_CONFIG`, `REQUEST_EXIT_FEED_MODE`
  **emitidos/declarados mas sem handler**.
- Tela de Feed nunca é auto-aberta; histórico de alertas nunca populado; `monthly_kwh[]` vazio.

### 2.6 Resiliência de storage (afeta notas 9/12)
- **Bug crítico:** `storage_sd_write_log` abre `.tmp` em `"a"`, escreve 1 linha e renomeia
  sobre `log.txt` → **trunca o log inteiro para 1 linha**.
- Sem detecção de remoção a quente do SD; mount só no boot; `audit_log.c` não usa o RAM fallback.

### 2.7 Correção de nomenclatura de ALM (para o plano ser preciso)
- **PF baixo = ALM-053** (RF-ENERGY-009). **ALM-057 = tendência de tensão**,
  **ALM-058 = tendência de corrente**. (Alinhar comentários/uso.)

---

## 3. Plano de ação para atingir ≥ 95% (todas as notas < 9.5)

> Organizado por workstream priorizado (P0 = segurança/bloqueante → P3 = polimento).
> Cada item lista RF/ALM, arquivos e critério de aceite auditável.

### P0 — Segurança e integridade (eleva notas 1, 2, 9)
| # | Ação | RF/ALM | Arquivos | Aceite |
|---|------|--------|----------|--------|
| P0-1 | Dono único de ALM (raise+clear) por ID | RF-ALERT-003/005 | `alm_monitor.c`, `electric_fsm.c`, `driver_ds18b20.c` | Grep prova 1 só raise por ID; sem donos duplos |
| P0-2 | Implementar auto-clear central lendo `alm_catalog.auto_clear` | RF-ALERT-003 | `alert_manager.c`, `alm_catalog.c` | ALMs INFO/WARN limpam ao normalizar (teste manual) |
| P0-3 | Escaladas: ALM-048→SAFE_OFF, ALM-029→EMERGENCY, corrigir mapa OV/UV→050/051 | §49, RF-GLOBAL | `safeoff_alm_map.c`, `alm_monitor.c`, `app_main.c` | Causa correta registrada em `safeoff_record` |
| P0-4 | Corrigir saída de EMERGENCY (causa/ALM certos, limpar latch térmico, `safeoff_record_resolve_latest`) | RF-GLOBAL-EMERG-EXIT-001 | `safety_controller.c`, `app_main.c`, `thermal_fsm.c` | Saída só com pré-condições reais atendidas |
| P0-5 | Corrigir bug de append de log (`.tmp` copia+append+rename) | RF-STORAGE-003, RF-PERSIST-POWERLOSS-002 | `storage_sd.c` | Log preserva histórico após N escritas |
| P0-6 | ALM-020/019 do ATO levantam mesmo sob `force_safe_off` | RF-ATO-OPER-003 | `ato_fsm.c`, `app_main.c` | Timeout/bloqueio geram ALM correto |

### P1 — Cabeamento funcional que "existe mas não roda" (eleva notas 5, 6, 13)
| # | Ação | RF | Arquivos | Aceite |
|---|------|----|----------|--------|
| P1-1 | Rotear Wizard no boot se `!wizard_completed`; concluir→DASHBOARD | RF-UI-WIZARD-001 | `ui_app.c`/`ui_screen_manager.c` | 1ª inicialização abre Wizard no passo persistido |
| P1-2 | Tratar `NAVIGATE_HOME/BACK`, `EXIT_FEED_MODE`, `SAVE_THERMAL_CONFIG` | RF-UI-NAV-001, RF-FEED-002 | `ui_events.c` | Botões Salvar/Cancelar/Sair funcionam |
| P1-3 | Auto-abrir tela de Feed + LED amarelo piscando durante feed | RF-FEED-002/003 | `app_main.c`, `ui_screen_feed_active.c` | Feed mostra tela e LED conforme SRS |
| P1-4 | `config_set_*` recarrega FSM viva (`*_fsm_set_config`) | RF-THERMAL-004/006, RF-ATO-003 | `app_main.c`, `config_manager.c` | Mudança de limite afeta controle sem reboot |
| P1-5 | Restauração de plugues no fim do Feed via `pre_feed_on_mask` | RF-FEED-001 | `app_main.c`, `plug_manager.c` | Só bombas que estavam ON religam |
| P1-6 | Alimentar `energy_wh_today` (cdn_energy→plug) p/ ALM-054 | RF-PLUG-013, RF-ENERGY-003 | `cdn_energy.c`, `plug_manager.c` | Limite diário dispara; reset à meia-noite |

### P2 — Proteção elétrica e religamento robustos (eleva notas 7, 8, 10)
| # | Ação | RF | Arquivos | Aceite |
|---|------|----|----------|--------|
| P2-1 | Proteção por-plug real: usar `current_limit_a`/`fator_curto`/`tempo_curto`; de-energiza+`blocked`+ALM | RF-PLUG-003/014 | `plug_manager.c`, `electric_fsm.c` | Sobrecorrente por plug atua e bloqueia |
| P2-2 | `restart_fsm_set_blocked_mask()` cabeado; ordem BOMBA-primeiro; skip bloqueados | RF-GLOBAL-REARM-001, RF-PROTECTION-001 | `app_main.c`, `restart_fsm.c` | Plug bloqueado não religa; ordem correta |
| P2-3 | Monitoramento de corrente por plug durante MONITORING; falha→bloqueia+ALM-056 | RF-FSM-RELIG-ELECT-001 | `restart_fsm.c`, `app_main.c` | Religamento valida corrente antes de manter ON |
| P2-4 | Enforce P01/P02 (bloquear modos/tipos indevidos; sair do Feed) | RF-PLUG-010 | `command_validator.c`, `plug_manager.c` | P01/P02 não aceitam TIMER; não entram no Feed |
| P2-5 | Persistência de energia (dia/mês/total) + custo (tarifa) + log elétrico estruturado | RF-ENERGY-002/003/005, RF-LOG-ELECTRIC-001 | `cdn_energy.c`, `electric_service.c`, `log_manager.c` | Totais sobrevivem a reboot; eventos elétricos logados |

### P3 — Contrato web, segurança e UX (eleva notas 11, 12, 13, 14)
| # | Ação | RF | Arquivos | Aceite |
|---|------|----|----------|--------|
| P3-1 | Corrigir IP do cliente (peer do socket) e unificar rate-limit | RNF-SECURITY-001 | `api_rest.c`, `api_rate_limit.c`, `api_auth.c` | Bloqueio por IP funciona por cliente; evento auditado |
| P3-2 | Exigir auth em `POST /auth/password`; min 8 chars | RF-WEB-004, RNF-SECURITY-002 | `api_auth.c`, `api_rest.c` | Sem troca de senha anônima |
| P3-3 | Códigos de erro canônicos aninhados (`error.code/message/details`) | RF-API-ERROR-CODES-001 | `api_rest.c` | Payload de erro conforme SRS |
| P3-4 | Auditoria estruturada (ip/user/result/event_type) via `storage_facade`; auditar negados | RF-AUDIT-SEC-001/002, RNF-SECURITY-003 | `audit_log.c`, `api_rest.c` | Todo write negado gera registro estruturado |
| P3-5 | Alinhar rotas/campos ao contrato (`/export`, `/import`, `/maintenance`, `/plugs/{id}`, `fw_version`, `active_alerts_count`) | RF-WEB-002/008, RNF-WEB-CONTRACT-001 | `api_rest.c` | Contrato bate com §23.1 |
| P3-6 | Serviço `maintenance_mode` (toggle+timeout+auto-exit) + UI + suspender proteções | RF-UI-MENU-003/003.1, RNF-INDUSTRIAL-004 | novo `services/maintenance_mode.c`, `ui`, `electric_fsm.c` | Manutenção com contagem regressiva e reativação |
| P3-7 | Perfis (salvar/carregar/renomear/excluir) em SD/NVS + UI | RF-UI-MENU-002, RF-PERSIST-PROFILE-001 | `config_export.c`/novo, `ui` | CRUD de perfis com CRC |
| P3-8 | Overlays por-causa (ocorreu/impacto/ação auto/recomendada/saída) + barra de status (manut/MUTE/NVS/wizard) | RF-UI-OVERLAY-001.1, RF-UI-STATUS-001 | `ui_critical_overlay.c`, `ui_topbar.c`/`ui_footer.c` | 12 causas com template; badges presentes |
| P3-9 | ATO digital (ADC→ON/OFF, debounce, calib) + aplicar offsets | RF-ATO-DIGITAL-001, RNF-CALIB-001 | `param_catalog.h`, `ato_fsm.c`, `app_main.c` | Modo digital documentado e funcional |

### P4 — Deduplicação (eleva "sem código duplicado")
| # | Ação | Arquivos |
|---|------|----------|
| P4-1 | Remover `alm_ctl.c`, `sec_policy` morto, `thermal_service`/`ato_service` órfãos | citados em §2.1 |
| P4-2 | Remover `drivers/ui_keypad.c`/`ui_touch.c` (fora do build) e `ui_status_bar.h` órfão | drivers/, ui/hmi/ |
| P4-3 | Consolidar buffers RAM de log e trilhas de export/backup/rollback | storage_* |
| P4-4 | Cabear ou remover componentes UI sem callers | ui/hmi/components |

---

## 4. Sequência recomendada de execução
1. **P0** (segurança/integridade) — pré-requisito para qualquer claim de "industrial-grade".
2. **P1** (cabeamento) — destrava valor já construído (wizard, feed, config runtime).
3. **P2** (elétrico/religamento) — robustez de proteção e recuperação de falha.
4. **P3** (web/segurança/UX) — contrato, hardening e usabilidade.
5. **P4** (dedup) — em paralelo, à medida que cada dono único é definido.

**Gate software:** reauditar por domínio até cada nota ≥ 9.5 (ver `COMPLIANCE_SCOPE.md`).
Build local recomendado como evidência, mas **fora** do placar. Hardware e testes Unity
ficam **após** o sign-off de software ≥ 95%.

---

## 5. Nota de honestidade
Este documento **não** ajusta números para atingir a meta. As auditorias de domínio
convergem em ~38–65% por área (~51% global). As RTMs anteriores marcaram itens como
"CONCLUÍDO" que, embora **implementados em código**, **não estão roteados/cabeados** em
runtime (ex.: wizard, feed, config→FSM) ou têm **ciclo de vida incompleto** (auto-clear de
ALM). O caminho para ≥95% é claro e está no plano acima; requer as execuções P0–P4 + build.

---

## 6. Execução autônoma P0–P4 + refinamentos (2026-07-01)

Ver `RTM_DELTA_COMPLIANCE_2026-07-01.md`. Resumo pós-sessões:

| Entregue | Itens |
|----------|-------|
| P0–P4 core | auto-clear ALM, storage append, escaladas, wizard boot, config→FSM, feed UI, plug protection, API §23, maintenance mode, perfis, overlays por-causa, log elétrico, ALM-025 mensal |
| Refinamentos | keypad, MUTE, diag drill-down, confirm dialog, RF-PLUG-003, badges, carrossel 15s configurável, pausa carrossel 5/10/15/30s, perfil teclado livre, desbloqueio plug (manutenção), API carousel/profile rename |

**Compliance estimada (software only): ~69%** — reauditoria completa em
`REAUDITORIA_COMPLIANCE_2026-07-01.md`. Gap **~26 pp** até ≥95%.

**Próximo passo:** Fases R1→R3 do plano de fechamento (NC-S/A/C/E/W/U).
