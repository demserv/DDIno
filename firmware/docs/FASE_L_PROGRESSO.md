# Fase L — Progresso de Compliance (build BYPASS)

Baseline: pós-Fase K · Build: **BYPASS** (não entra na nota)

## L01 — NC-S09b restart → enter_normal
- Status: **COMPLIANT**
- Evidência: `main/app/app_main.c:238-247` — `global_state_enter_normal(&g_gs, "restart_complete")`; sem atribuição direta no bloco restart
- Teste estático: N/A
- Bloqueio: (vazio)

## L02 — NC-CODE-01 header duplicado
- Status: **COMPLIANT**
- Evidência: `core/safety_controller.h:46` — uma única declaração de `global_state_enter_normal`
- Teste estático: N/A
- Bloqueio: (vazio)

## L03 — NC-S09c anti-flap NVS único
- Status: **COMPLIANT**
- Evidência: `core/global_state.c:67-88` (`global_state_antiflap_allow/commit`); `core/safety_controller.c:147-151` usa config; zero `HW_ANTIFLAP_*` em safety_controller.c
- Teste estático: `test/test_safety.c` (existente)
- Bloqueio: (vazio)

## L04 — NC-S09 global_state_transition unificado
- Status: **COMPLIANT**
- Evidência: `core/global_state.c:189-211` — case NORMAL delega `global_state_enter_normal` + antiflap; `core/safety_controller.c:107` — `event_bus_publish` em enter_normal
- Teste estático: `test/test_safety.c` (existente)
- Bloqueio: (vazio)

## L05 — NC-A07b sync g_gs pós-import
- Status: **COMPLIANT**
- Evidência: `core/global_state.c:91-105` (`global_state_sync_from_config`); `services/config_export.c:348-352` chama após commit
- Teste estático: N/A
- Bloqueio: (vazio)

## L06 — NC-A07 ACS712 no JSON
- Status: **COMPLIANT**
- Evidência: `services/config_export.c:117-122` export array; `287-301` import com validação tamanho 10
- Teste estático: N/A
- Bloqueio: (vazio)

## L07 — NC-FEED-01 feed_cooldown default
- Status: **COMPLIANT**
- Evidência: `services/config_manager.c:123-124` — `feed_cooldown_min = 0` documentado (SRS não define valor; 0 = cooldown desabilitado em `feed_fsm.c`)
- Teste estático: N/A
- Bloqueio: (vazio)

## L08 — NC-A10b rate limit docs
- Status: **COMPLIANT**
- Evidência: `web/api_rate_limit.c:1-2`, `web/api_rate_limit.h:1-2`; login separado em `web/api_auth.c:159-179`; include morto removido de api_auth
- Teste estático: N/A
- Bloqueio: (vazio)

## L09 — NC-I02 circuit breaker drivers
- Status: **COMPLIANT**
- Evidência: `driver_mcp23017.c`, `driver_mcp3208.c`, `driver_ds3231.c`, `driver_ds18b20.c`, `driver_pzem.c`, `driver_xpt2046.c`, `driver_ili9488.c` — `circuit_breaker_record_*` / `is_available`
- Teste estático: N/A
- Bloqueio: (vazio)

## L10 — NC-U13 telas órfãs
- Status: **COMPLIANT**
- Evidência: `screen_submenu.c` removido; `screen_calibration.c` ausente do tree; zero referências em CMake/sources
- Teste estático: N/A
- Bloqueio: (vazio)

## L11 — NC-U14 ACK individual UI
- Status: **COMPLIANT**
- Evidência: `ui_events.c:47-56` (`ui_events_ack_alert`); `ui_alert_row.c:8-14` click; `ui_screen_alerts.c:128-129` enable por linha ativa
- Teste estático: N/A
- Bloqueio: (vazio)

## L12 — NC-BOOT-01 política boot self-test
- Status: **COMPLIANT**
- Evidência: `app_main.c:1120-1134` — crítico→SAFE_OFF; não-crítico→DEGRADED documentado; alinhado a `self_test.c` matriz `s_critical[]` e ALM-063 SRS (CRITICAL vs HIGH)
- Teste estático: N/A
- Bloqueio: (vazio)

## L13 — NC-I04 threshold WDT documentado
- Status: **COMPLIANT**
- Evidência: `include/hardware_config.h:125-127` — comentário engineering default + observabilidade SRS
- Teste estático: N/A
- Bloqueio: (vazio)

## L14 — RTM + relatório final
- Status: **COMPLIANT**
- Evidência: `docs/RTM.md` seção Fase L; `docs/FASE_L_RELATORIO.md`
- Teste estático: N/A
- Bloqueio: (vazio)
