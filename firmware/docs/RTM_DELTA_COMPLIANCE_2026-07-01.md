# RTM Delta / Auditoria de Compliance — Execução do Plano 95% (SRS v3.11)

> Documento honesto de rastreabilidade cobrindo **o delta implementado nas Fases 0–4**
> do plano de compliance. Complementa (não substitui) `COMPLIANCE_REPORT.md` e a
> `AUDITORIA_SRS.md`. Itens deferidos estão listados explicitamente com justificativa.
>
> **Data:** 2026-07-01 · **Baseline:** SRS v3.11 + Adendo pH v3.11 + Errata §49.1 (ALM-062)

---

## Fase 0 — Normalização do sensor de pH na baseline

| RF | Descrição | Arquivos | Status |
|----|-----------|----------|--------|
| RF-PH-001..004 | pH como telemetria normativa: leitura, faixas de advertência configuráveis, calibração via **ALM-049** (canônico), **sem SAFE_OFF** e **sem usar ALM-038/039** (que são do ATO) | `docs/ADENDO_BASELINE_PH_v3.11.md`, `include/param_catalog.h` (`ph_params_storage_t`), `services/config_root.h`, `services/config_manager.c/.h`, `services/config_export.c` | COMPLETED |
| RF-GLOBAL-005 | pH deixa de ser hardcoded: parâmetros migrados para o ConfigRoot (NVS + CRC + schema) | `include/param_catalog.h`, `include/hardware_config.h` (removidos `HW_PH_*` fixos) | COMPLETED |

## Fase 1 — Semântica canônica de ALMs (SRS §49)

| ALM | Correção / Implementação | Owner (raise+clear) | Status |
|-----|--------------------------|---------------------|--------|
| ALM-003 | Reset por Watchdog (HIGH) | `services/alm_monitor.c` | COMPLETED |
| ALM-022 | Falha de comunicação PZEM | `services/electric_fsm.c` | COMPLETED |
| ALM-027 | Nível baixo persistente (DEGRADED) | `fsm/ato_fsm.c` | COMPLETED |
| ALM-029 | Excesso de resets em 24h (CRITICAL) via `wdt_stats` | `services/alm_monitor.c` | COMPLETED |
| ALM-030 | Bypass/corrente com plug OFF (antes mapeado como 054) | `services/plug_manager.c` | COMPLETED |
| ALM-035 | Feed mode abortado | `main/app/app_main.c` | COMPLETED |
| ALM-038 | Frequência anormal de refill (janela configurável) | `fsm/ato_fsm.c` | COMPLETED |
| ALM-039 | Reservatório vazio/bloqueio (bomba ativa sem subida de nível) | `fsm/ato_fsm.c` | COMPLETED |
| ALM-049 | pH fora da faixa calibrada (WARNING, sem SAFE_OFF) | `services/alm_monitor.c` | COMPLETED |
| ALM-054 | Limite diário de energia por plug (antes sem ALM/054 mal-mapeado) | `services/plug_manager.c` | COMPLETED |
| ALM-057 | Tendência de tensão para fora da faixa (pré-alarme WARNING) | `services/alm_monitor.c` | COMPLETED |
| ALM-061 | ConfigRoot inválido: rejeição de import + rollback | `services/config_export.c` | COMPLETED |
| ALM-062 | Acesso web não autorizado → **WARNING** (auto-clear, sem ACK, **não escala p/ SAFE_OFF**) | `web/api_auth.c`, `services/alm_catalog.c`, `services/alert_manager.c` | COMPLETED (Errata §49.1) |

**Centralização:** ALM-050/051/053/057/058 têm raise+clear centralizados em `alm_monitor.c`
(com histerese), evitando duplicação com `electric_fsm.c`/`cdn_energy`.

### Deferidos da Fase 1 (com justificativa)
- **ALM-025 (limite mensal):** exige acumulador mensal persistente + RTC confiável;
  não há infraestrutura de janela mensal. Requer novo serviço de acumulação.
- **Dedup 050/051/053/013:** ocupam slot único de alerta; a duplicação é apenas de
  código de leitura, sem divergência comportamental. Refator sem ganho funcional.

## Fase 2 — Wizard inicial (SRS §72.5)

| RF | Descrição | Arquivos | Status |
|----|-----------|----------|--------|
| RF-UI-WIZARD-004 | Ordem oficial de **16 etapas** (enum expandido + `WIZARD_TOTAL_STEPS`) | `include/global_state.h` | COMPLETED |
| RF-UI-WIZARD-001..005 | 16 telas de revisão dos grupos críticos; reentrada segura (passo em NVS); commit só no CONFIRM | `main/ui/hmi/screens/ui_screen_wizard.c` | COMPLETED |
| RF-UI-WIZARD-002 | Seleção interativa **127/220 V** (ajusta OV/UV e persiste `mains_voltage`) | `ui_screen_wizard.c`, `config_manager` | COMPLETED |
| RF-UI-WIZARD-00X.1 | Validação bloqueante com feedback (térmica/ATO/elétrica) | `ui_screen_wizard.c` | COMPLETED |

## Fase 3 — Menus §73, reset, overlay

| RF | Descrição | Arquivos | Status |
|----|-----------|----------|--------|
| SRS §73.1 | Menu principal alinhado (Diagnósticos + Manutenção; removido "Rede/WiFi" mal-rotulado e alvo duplicado de Sistema) | `main/ui/hmi/screens/ui_screen_main_menu.c` | COMPLETED |
| RF-UI-MENU-001 / RF-RESET-002 | **Reset de fábrica por menu** conectado à FSM segura de duplo-confirm (`FACTORY_RESET`→start/confirm) | `main/ui/hmi/ui_events.c`, `ui_screen_system.c`, `services/reset_handler.c` | COMPLETED |
| RF-RESET | Reboot controlado por menu (`SAFE_REBOOT`→`esp_restart`) | `ui_events.c`, `ui_screen_system.c` | COMPLETED |
| RF-UI-OVERLAY-001 | **ACK inline** no overlay crítico (SAFE_OFF/EMERGENCY) sem navegar até Alertas | `main/ui/hmi/components/ui_critical_overlay.c/.h` | COMPLETED |
| — | Correção: tile "Manutenção" abria o Wizard por engano; typo "Ver does"→"Logs" | `ui_screen_system.c` | COMPLETED |

### Deferidos da Fase 3 (com justificativa)
- **RF-UI-MENU-003.1 (Modo Manutenção c/ timeout configurável, auto-exit):** exige um
  serviço dedicado `maintenance_mode` (timer, notificação pré-expiração, reativação de
  proteções). Mexe em comportamento safety-adjacent; requer build + validação.
- **RF-UI-MENU-002 (Perfis: salvar/carregar/renomear/excluir):** exige slots de perfil
  em storage. **Backup/restauração JSON (RF-UI-MENU-002.1)** já está coberto por
  `config_export.c` (export/import + validação + rollback + ALM-061).

## Fase 4 — Deduplicação

| Ação | Detalhe | Status |
|------|---------|--------|
| Remoção de código morto | `screen_calibration.c` e `screen_submenu.c` (órfãos pré-port, sem header, **não listados no CMake**, 0 referências; superados por `ui_screen_config_temperature.c`/`ui_screen_calibration.c`) | COMPLETED |
| `ili9488_sprint` | Não existe em fonte (apenas artefatos de build/docs) — nada a remover | N/A |

### Deferidos da Fase 4 (com justificativa)
- **`driver_ad_keypad.c` (raw):** `ad_keypad_read`/callback são dead-code; apenas
  `ad_keypad_init(NULL)` é chamado no `app_main`. O caminho ativo de teclado é o
  adaptador LVGL (`driver_ad_keypad_lvgl.c`). Remover exige tocar a sequência de init
  do `app_main` (safety-critical) e unificar thresholds divergentes
  (`THRESH_*` em cascata vs janela `ADC_UP_THRESH_MIN/MAX`) — requer build + validação
  de navegação.

---

## Gate de build e escopo

- **Escopo ≥ 95%:** ver `docs/COMPLIANCE_SCOPE.md` — **sem** testes Unity nem hardware.
- **Build:** evidência recomendada via `firmware/_run_build.ps1` →
  `build-test3/monitor_aquario_inteligente_fw.bin`. Não entra no placar de compliance.
- **Hardware / flash / smoke:** somente **após** sign-off software ≥ 95% SRS.

## Observações de honestidade

- `COMPLIANCE_REPORT.md` contém seções legadas (wizard "6 steps", `ui/ui_screens.c`)
  anteriores à reestruturação do HMI; os mapeamentos ALM de bypass (030) e energia/dia
  (054) foram corrigidos nele nesta passada.
- Itens marcados **DEFERIDO** não são gaps ignorados: têm dependência de novo módulo,
  RTC/acumulador, ou validação de hardware/navegação, e estão registrados para a
  passada com ambiente de build.
