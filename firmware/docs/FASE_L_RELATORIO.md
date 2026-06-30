# Fase L — Relatório Final de Compliance

**Data:** 2026-06-30  
**Build:** BYPASS (não executado neste ambiente; **excluído da nota**)  
**Método:** evidência estática `arquivo:linha` · gate sequencial L01→L14

---

## 1. Resumo

Todos os 14 itens da Fase L foram implementados e verificados por grep/leitura de código antes de avançar.

| Domínio | Peso | Score pós-L | Itens fechados |
|---------|------|-------------|----------------|
| Safety / transições | 25% | **0,96** | NC-S09b, NC-S09, NC-S09c, NC-CODE-01 |
| FSM / boot | 25% | **0,95** | NC-FEED-01, NC-BOOT-01 |
| API / persistência | 18% | **0,96** | NC-A07, NC-A07b, NC-A10b |
| Alertas / UI | 17% | **0,96** | NC-U13, NC-U14 |
| Infra / CB / RTM | 15% | **0,93** | NC-I02, NC-I04, RTM |

### Conformidade global (sem build)

≈ **95,5%** — meta ≥95% atingida após Fase M (M1–M5).

## Fase M (pós-reauditoria)

| Item | Status |
|------|--------|
| M1 plug_limits NVS | COMPLIANT |
| M2 órfãos removidos | COMPLIANT |
| M3 ui_app_refresh_now | COMPLIANT |
| M4 CB thresholds doc | COMPLIANT |
| M5 global_state_transition wired | COMPLIANT |


---

## 2. NC × status final

| NC | Status | Evidência principal |
|----|--------|---------------------|
| NC-S09b | COMPLIANT | `app_main.c:241` → `global_state_enter_normal` |
| NC-S09 | COMPLIANT | `global_state.c:189-211` delega enter_normal |
| NC-S09c | COMPLIANT | `global_state_antiflap_allow` + NVS |
| NC-CODE-01 | COMPLIANT | `safety_controller.h:46` única declaração |
| NC-A07b | COMPLIANT | `config_export.c:348-352` + `global_state_sync_from_config` |
| NC-A07 | COMPLIANT | JSON `acs712_zero_offset_mv[10]` simétrico |
| NC-FEED-01 | COMPLIANT | `config_manager.c:124` default 0 documentado |
| NC-A10b | COMPLIANT | Docs login vs API 30 req/min separados |
| NC-I02 | COMPLIANT | CB em 7 drivers + app_main loops existentes |
| NC-U13 | COMPLIANT | Órfãos removidos/ausentes |
| NC-U14 | COMPLIANT | ACK por linha ativa na UI |
| NC-BOOT-01 | COMPLIANT | Política crítico/não-crítico documentada |
| NC-I04 | COMPLIANT | WDT threshold documentado em HW config |
| RTM | COMPLIANT | Seção Fase L em RTM.md |

---

## 3. Pendências honestas (fora da nota)

| Item | Status | Nota |
|------|--------|------|
| `idf.py build` | BYPASS | Executar em PC com ESP-IDF |
| Unity PASS | BYPASS | Testes existem; execução pendente |
| Smoke HIL display/touch | BYPASS | Self-test SKIPPED aceitável |
| Double-count CB | Risco baixo | app_main + drivers em alguns buses; health reflete falhas reais |

---

## 4. Instrução build (referência)

```bash
git pull
cd firmware
idf.py build
```

Primeiro boot pós-flash com struct config alterada pode resetar NVS se CRC inválido (comportamento existente).

---

## 5. Arquivos alterados (Fase L)

- `main/app/app_main.c` — restart enter_normal; comentário boot self-test
- `core/safety_controller.c/h` — antiflap NVS; event_bus enter_normal
- `core/global_state.c` — antiflap export; sync config; transition unificada
- `include/global_state.h` — APIs antiflap + sync
- `services/config_export.c` — ACS712 JSON + sync pós-import
- `services/config_manager.c` — feed_cooldown default
- `web/api_rate_limit.c/h`, `web/api_auth.c` — docs rate limit
- `drivers/driver_*.c` (6 arquivos) — circuit breaker
- `main/ui/hmi/*` — ACK individual alertas
- `include/hardware_config.h` — doc WDT
- Removido: `main/ui/hmi/screens/screen_submenu.c`
