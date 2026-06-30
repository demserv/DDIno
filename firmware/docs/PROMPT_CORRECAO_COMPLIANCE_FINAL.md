# PROMPT MESTRE DE CORREÇÃO TOTAL — COMPLIANCE >= 95%
## PROJETO: MONITOR AQUÁRIO INTELIGENTE — DDINO (ESP32-S3 / ESP-IDF / C/C++ / LVGL)
## IDIOMA NORMATIVO: PT-BR

====================================================================
0. EQUIPE E PAPÉIS (TODOS OBRIGATÓRIOS)
====================================================================

Esta correção é executada por uma equipe multidisciplinar. Cada papel tem
responsabilidade explícita e poder de veto sobre o código que sai.

- AUDITOR MESTRE (sistemas embarcados, C/C++, safety-critical):
  - Revisa TODO código gerado contra a SRS e contra a auditoria
    `docs/` (AUDITORIA). Nenhuma correção é dada como concluída sem o
    parecer do auditor: COMPLIANT com evidência executável/rastreável.
  - Veta qualquer invenção (GPIO, threshold, tempo, estado, ALM, endpoint).

- ANALISTA DE SISTEMAS (C++ / ESP32 / ESP-IDF):
  - Garante coerência arquitetural: rota única de relés, FSMs, command path,
    persistência, watchdog, circuit breaker, health matrix.
  - Garante que cada alteração compila e não quebra contratos existentes.

- PROGRAMADORES (C++ / ESP32):
  - Implementam as correções item a item, com edição mínima e segura,
    preservando conteúdo válido existente.

- ANALISTA DE UI/UX (LVGL / HMI):
  - Confere telas, navegação por keypad sem touch, Feed/MUTE/ACK,
    legibilidade (cor + texto/ícone), temperatura primária, overlays
    de estado crítico, acessibilidade daltônica.

====================================================================
1. REGRAS ABSOLUTAS — ZERO INVENÇÃO / ZERO DELÍRIO
====================================================================

É PROIBIDO inventar: GPIOs, thresholds, tempos quantitativos, estados
globais, ALM IDs, mensagens ALM, endpoints, funções/estruturas
inexistentes, comportamento safety-critical não especificado, fabricantes,
SKUs, preços, critérios metrológicos.

- Se um dado não existir na SRS nem no código: registrar exatamente
  `PENDENTE — DADO NÃO FORNECIDO NO DOCUMENTO FONTE`.
- É PROIBIDO marcar COMPLIANT sem comportamento implementado e verificável.
- É PROIBIDO remover safety para o build passar.
- É PROIBIDO editar LVGL em `managed_components/lvgl__lvgl` (exceto restaurar
  versão oficial).
- Toda alteração deve manter o build compilável; se o build não puder ser
  executado nesta máquina (sem ESP-IDF), registrar a limitação e garantir
  coerência estática (sem símbolos indefinidos, sem warnings -Werror óbvios).
- Edição mínima: não reescrever módulos inteiros sem necessidade.

====================================================================
2. FONTES NORMATIVAS (ORDEM DE PREVALÊNCIA)
====================================================================

1. BLOCO 13/N  2. BLOCO 12/N  3. BLOCO 11/N  4. SRS Técnico Consolidado
v3.11-AF.3  5. AF.3 Pinagem/BOM  6. SRS v3.11 COMPLETA — GO CODING READY
7. Histórico não conflitante.

Arquivos: `DDIno/SRS TÉCNICO CONSOLIDADO FINAL — MONITOR AQUÁRIO INTELIGENTE.txt`
e `DDIno/SRS v3.11 COMPLETA — GO CODING READY.txt`.

====================================================================
3. NÃO CONFORMIDADES A CORRIGIR (DA AUDITORIA) — TODAS OBRIGATÓRIAS
====================================================================

P0 (bloqueadores):
- NC-001 BUILD BREAK: `electric_fsm_force_safe_off` indefinida
  (`services/electric_service.c:94`; declarar/definir em `electric_fsm.*`
   OU remover usando `out.force_safe_off`). Critério: compila; símbolo resolvido.
- NC-002 Bomba ATO desacoplada da FSM: `pump_request_on` ignorado; em AUTO
  `plug_manager` força BOMBA ON. Critério: bomba não liga em
  BLOCKED/ERROR/OVERFLOW; segue a `ato_fsm`.
- NC-003 `POST /api/v1/plugs` aciona relé crítico sem dupla confirmação.
  Critério: P01/P02 ON sem `confirm` → 409/negado.

P1:
- NC-004 Sem indev LVGL: `driver_xpt2046_init()` e
  `driver_ad_keypad_lvgl_init()` não são chamados no boot. Critério:
  navegação por keypad com touch desconectado; touch registrado.
- NC-005 `ui_events.c` é stub: FEED(confirmação)/MUTE/ACK inoperantes.
  Critério: FEED pede confirmação; MUTE só silencia (não toca FSM/ALM/LED/relé);
  ACK funciona.
- NC-006 Escritas sem validator/auth: `/config/monitor`, `/wizard` (sem auth),
  `command ack_all`, `config.wizard_completed`. Critério: toda escrita exige
  auth + validação.
- NC-007 Self-test de relé é stub (`self_test.c` `test_relay` sempre passa).
  Critério: detectar MCP23017/relé reflete em `selftest_passed`.
- NC-008 Health matrix não integrada (`health_report()` sem callers).
  Critério: matriz alimentada e exposta em `/api/v1/health`.
- NC-009 FSM elétrica: `total_current_limit_a`/`tempo_deteccao_curto_ms` não
  usados; bypass não ligado (`plug_manager_set_plug_current` nunca chamado);
  instância dupla de FSM. Critério: limites totais/tempo e bypass funcionam;
  instância única.
- NC-011 Circuit breaker: thresholds hardcoded; `CB_BUS_I2C`/`CB_BUS_SPI_SD`
  sem integração; `recover_timeout_ms` não usado.

P2:
- NC-010 FSM térmica não usa `temp_max_c`; média válida com 2 amostras (esperado 3);
  `temp_min/max` não carregados da NVS.
- NC-012 Import (`storage_import_from_buffer`) retorna NOT_SUPPORTED; sem preview.
- NC-013 `observation_mode` declarado e não usado.
- NC-015 `GET /api/v1/state` público.
- NC-016 Hash de senha sem salt; usuário `admin` fixo.

P3:
- NC-014 RTM com itens imprecisos (cita `ui/ui_display.c` inexistente; COMPLIANT
  não ligados). Corrigir RTM para evidência real.

====================================================================
4. ORDEM DE EXECUÇÃO
====================================================================

Sprint 1 (P0): NC-001, NC-002, NC-003.
Sprint 2 (P1 command/UI): NC-006, NC-005, NC-004.
Sprint 3 (P1 infra): NC-007, NC-008, NC-009, NC-011.
Sprint 4 (P2): NC-010, NC-012, NC-013, NC-015, NC-016.
Sprint 5 (P3 + fechamento): NC-014, RTM/relatório, checklist final.

====================================================================
5. CRITÉRIO DE ACEITE GLOBAL
====================================================================

- Sem símbolo indefinido (build coerente).
- Nenhuma função P0 vazia/stub safety-critical.
- LVGL íntegro; indev (touch+keypad) registrado.
- SAFE_OFF/EMERGENCY desligam relés; nenhuma rota burla safety.
- Bomba ATO segue a FSM.
- Toda escrita de API exige auth + command_validator.
- RTM aponta evidência real.
- Relatório final `docs/CORRECAO_COMPLIANCE_SRS_FINAL.md` atualizado.
- Cada item revisado e aprovado pelo AUDITOR MESTRE.

FIM DO PROMPT.
