# Reauditoria Final de Compliance — SRS v3.11 (Software Only)

> **Data:** 2026-07-01 · **Baseline:** SRS v3.11 + Adendo pH + Errata §49.1  
> **Escopo:** `docs/COMPLIANCE_SCOPE.md` — **somente** código + wiring em runtime  
> **Excluído do placar (permanente):** smoke, flash, E2E físico, Unity, `test/`, CI de testes, build CI  
> **Método:** L3 — evidência `arquivo:função`, caller de produção, sem stub/código morto  
> **Pós:** Fases E–G + H1–H3 + limpeza de stubs + **P1 completo**

---

## 1. Veredito executivo

| Métrica | Resultado | Meta | Status |
|---------|:---------:|:----:|:------:|
| **Compliance global ponderado** | **~94%** | ≥95% | **REPROVADO (margem ~1 pp)** |
| **Média aritmética (14 domínios)** | **~9,4/10** | ≥9,5/domínio | **REPROVADO (margem)** |
| **Domínios ≥ 9,5/10** | **6 / 14** | 14 / 14 | — |
| **Gap até sign-off software** | **~1 pp** | — | — |

**Smoke manual e testes automatizados não entram neste placar** e só ocorrem **após** sign-off software ≥95% (decisão do Owner).

---

## 2. Placar por domínio (função a função)

| # | Domínio | Nota | Evidência principal | Gap remanescente |
|---|---------|:----:|---------------------|------------------|
| 1 | Safety — SAFE_OFF / EMERGENCY / ACK | **9,4** | `safety_controller.c`; double-ACK plug-instance | Exit EMERGENCY estreito (`app_main.c:441-445`) |
| 2 | Alertas / ALM — catálogo §49 | **9,5** | ALM-064 WARN; ALM-058 tendência; `fsm_alm_sync` | ACK não logado em SD (só raise/clear) |
| 3 | RF-ALERT-001..006 | **9,5** | `related_plug_id` em ACK; SD ALERT log | — |
| 4 | Térmico | **9,3** | `thermal_fsm.c`; ALM visível em SAFE_OFF | ALM FSM suprimido sob `force_emergency` |
| 5 | ATO | **9,2** | `ato_fsm.c`; ALM ATO visível em SAFE_OFF | — |
| 6 | Feed | **9,0** | `feed_fsm.c`; snapshot/restore; ALM-035 abort | — |
| 7 | Elétrico / Energia | **9,4** | ALM-058 tendência de corrente | — |
| 8 | Plugues | **9,2** | OC block; ALM-030/041/054/056; UI toggle | — |
| 9 | Config / Persistência / Storage | **9,3** | ConfigRoot CRC; `config_set_*` + event bus | — |
| 10 | Resiliência / Religamento | **9,0** | `restart_fsm.c`; blocked_mask | ALM-056 restart sem auto-clear pós-retry |
| 11 | Web API §23 | **9,5** | `read_requires_auth` uniforme em todos GETs | TLS/PBKDF2 deferidos |
| 12 | Segurança (auth / audit) | **9,2** | Token + rate-limit; wizard setup exempt | Senha SHA-256 (não PBKDF2) — deferido |
| 13 | UI / HMI funcional | **9,2** | Wizard boot; overlays; device pages | Alguns componentes UI sem caller |
| 14 | UX (operação diária) | **9,5** | Topbar `MNT{N}m`; tela system pre-expiry | — |

**Média:** 131,2 / 14 = **9,37/10 (~94%)**

---

## 3. Placar ponderado (5 buckets)

| Bucket | Peso | Score | Contribuição |
|--------|:----:|:-----:|:------------:|
| Segurança / SAFE_OFF / ACK (+ auth) | 30% | 93,5% | 28,05 pp |
| FSM global + boot + restart + feed | 25% | 92,0% | 23,00 pp |
| Alertas / ALM / RF-ALERT | 15% | 95,0% | 14,25 pp |
| API / Web / persistência / energia | 15% | 94,0% | 14,10 pp |
| UI / HMI / UX funcional | 15% | 93,5% | 14,03 pp |
| **Total** | 100% | | **~93,4%** |

---

## 4. H1–H3 — verificação L3

| Item | Status | Evidência |
|------|:------:|-----------|
| SD log alertas (`SD_LOG_TYPE_ALERT`) | OK | `alert_manager.c:65-76`, `:193`, `:205` |
| POST `/alerts` ACK por instância | OK | `api_rest.c:933-946` |
| RF-PH-002 warn band | OK | `alm_monitor.c:355-372`; `api_rest.c:747-750` |
| `config_set_*` → event bus | OK | `config_manager.c:85-90`, `:311-428`; `app_main.c:192-198` |
| ALM-056 por plug | OK | `plug_manager.c:76-80`; `app_main.c:401-410` |
| ALM-064 clear | OK | `alm_monitor.c:560-561`; `maintenance_mode.c:47` |
| `max_energy_wh_day` persist + API | OK | `param_catalog.h`; `plug_manager.c:636-645`; `api_rest.c:2042-2050` |
| pH em `/sensors` | OK | `api_rest.c:731-753` |
| CORS rotas novas | OK | `api_rest.c:2131+` |
| Manutenção pre-expiry | OK | `maintenance_mode.c:61-75`; `ui_topbar.c`; `ui_screen_system.c` |

---

## 5. P1 — verificação L3

| ID | Status | Evidência |
|----|:------:|-----------|
| P1-UX-01 | OK | `maintenance_mode_is_expiring_soon()`; topbar `MNT{N}m`; system screen hint |
| P1-ALM-01 | OK | `alert_manager_ext_ack_critical_instance()`; `related_plug_id` em ACK |
| P1-ALM-02 | OK | `alm_monitor.c:576` — ALM-064 `ALERT_SEVERITY_WARNING` |
| P1-ALM-03 | OK | `alm_monitor.c:500` — ALM-058 tendência de corrente |
| P1-FSM-01 | OK | `app_main.c:117-128` — `fsm_alm_sync` sem supressão SAFE_OFF |
| P1-API-01 | OK | `api_rest.c:auth_guard` — `read_requires_auth` em **todos** GETs |

**P0:** nenhum identificado pós-P1.

---

## 6. P2 restantes (~1 pp até ≥95%)

| ID | Ação | Domínio |
|----|------|---------|
| P2-SAF-01 | Exit EMERGENCY mais amplo (condições §49) | Safety |
| P2-ALM-01 | Log SD de ACK (além raise/clear) | RF-ALERT |
| P2-FSM-01 | ALM térmico sob `force_emergency` | Térmico |
| P2-RES-01 | ALM-056 auto-clear pós-retry restart | Resiliência |

---

## 7. Critério de sign-off

1. Cada domínio §2 ≥ **9,5/10**
2. Cada bucket §3 ≥ **95%**
3. Global ponderado ≥ **95%**
4. **Depois:** montagem hardware → flash → smoke manual (fora do placar)
