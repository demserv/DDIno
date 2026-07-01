# Audit Scope — Monitor Aquário Inteligente

## Version

| Field | Value |
|-------|-------|
| Document Version | 1.0 |
| Date | 2026-06-27 |
| Firmware Version | compliance-fix-v1 |
| SRS Baseline | Técnico Consolidado Final v3.11-AF.3 + AF.4 + Bloco 12/N + Bloco 13/N |

## Compliance Score Scope (2026-07-01)

> **Política canônica:** `docs/COMPLIANCE_SCOPE.md`

Para o **placar oficial ≥ 95% SRS**, contam apenas implementação e cabeamento de software.
**Não contam:** montagem de hardware, flash/smoke/E2E físico, execução de suítes Unity em
`test/`, nem CI de testes. Hardware só entra **após** sign-off de software ≥ 95%.

---

## Scope Boundaries

### In Scope (audit trail — código)

The following directories and file types are included in compliance audit:

| Directory | Includes | Excludes |
|-----------|----------|----------|
| `main/` | All `.c`, `.h`, `CMakeLists.txt`, `idf_component.yml` | — |
| `core/` | All `.c`, `.h` | — |
| `drivers/` | All `.c`, `.h` | — |
| `services/` | All `.c`, `.h` | — |
| `fsm/` | All `.c`, `.h` | — |
| `hal/` | All `.c`, `.h` | — |
| `web/` | All `.c`, `.h` | — |
| `ui/` | All `.c`, `.h` | — |
| `security/` | All `.c`, `.h` | — |
| `include/` | All `.h` | — |
| `test/` | All `.c`, `CMakeLists.txt` | For **code audit** only — **excluded from ≥95% score** |
| `docs/` | All `.md` | — |
| `scripts/` | All files | — |
| Root config files | `partitions.csv`, `sdkconfig.defaults`, `CMakeLists.txt` | — |

### Out of Scope (≥ 95% score)

| Item | Reason |
|------|--------|
| Montagem / bancada / flash / smoke test | Gate **após** software ≥ 95% (`COMPLIANCE_SCOPE.md`) |
| Execução Unity / `idf.py test` / CI de testes | QA opcional; não reduz placar de software |
| E2E físico (relés, sensores, touch) | Requer hardware montado |

### Out of Scope (audit trail)

| Path | Reason |
|------|--------|
| `managed_components/` | Third-party vendor code (LVGL, ESP-IDF managed) — not subject to project audit; version/pinned reference recorded in THIRD_PARTY_COMPONENTS.md |
| `build/` | Build artifacts — regenerated |
| Unity / CMock runners | Third-party test infrastructure |
| `concatena.ps1` (root) | Developer utility script — not part of firmware deliverable |
| Any `*.bin`, `*.elf`, `*.map`, `*.o` | Build artifacts |
| Any `*.pyc`, `__pycache__/` | Python cache |
| IDE/editor files (`.vscode/`, `.idea/`, `*.swp`) | Developer environment |

## Audit Criteria

### Mandatory Requirements (P0) — Blocking

| Criterion | Description |
|-----------|-------------|
| BUILD | `idf.py build` completes with zero errors, zero warnings (`-Wall -Wextra -Werror`) |
| SAFE_OFF | All 10 rules: relay_all_off, reason/timestamp/source ALM, audit log, API state, UI overlay, command blocking, diagnostic preservation |
| EMERGENCY | Latching with reason, relay_all_off, manual reset only |
| GLOBAL_STATE | Thread-safe transitions with priority validation (EMERGENCY > SAFE_OFF > DEGRADED > NORMAL) |
| COMMAND_VALIDATOR | All write paths gated by `can_*` checks |
| NO_BYPASS | UI/API never calls `relay_set()`, `gpio_set_level()`, `mcp23017_write_gpio()` directly |
| ZERO_HARDCODE | All operational values via ConfigRoot / param_catalog.h |
| AUTH | No hardcoded credentials; SHA-256 hashed password in NVS; session tokens in RAM |
| STORAGE_ATOMIC | Critical writes use `.tmp` + fsync + rename; RAM fallback when SD absent |

### Recommended Requirements (P1) — Non-blocking

| Criterion | Description |
|-----------|-------------|
| FREE RTOS TASKS | Dedicated tasks with priorities (not single-loop) |
| UNIT TESTS | `idf.py test` passes — **recommended post-95%**, not part of official score |
| RTM_COVERAGE | 100% of SRS requirements mapped in RTM.md |
| COMPLIANCE_REPORT | SRS_COMPLIANCE_IMPLEMENTATION_REPORT.md with evidence for every requirement |
| WDT_COVERAGE | Task watchdog or advanced WDT for all critical paths |

### Out of Scope Requirements

| Requirement | Status | Reason |
|-------------|--------|--------|
| RF-OTA-001..005 | Deferred | Post-compliance epic |
| RF-EMAIL-001 (SMTP) | Deferred | Future epic |
| RF-PH-001 (pH sensor) | Deferred | Future epic |
| RF-SENSOR-REDUNDANCY-001 | Deferred | Future epic |

## Audit Artifacts

| Artifact | Location | Status |
|----------|----------|--------|
| RTM | `docs/RTM.md` | ✅ Exists |
| FREERTOS_TASK_MAP | `docs/FREERTOS_TASK_MAP.md` | ✅ Exists |
| SRS Compliance Report | `docs/SRS_COMPLIANCE_IMPLEMENTATION_REPORT.md` | ✅ Exists |
| TEST_PLAN | `docs/TEST_PLAN.md` | ⚠️ Pending creation |
| BUILD_EVIDENCE | `docs/BUILD_EVIDENCE.md` | ⚠️ Pending creation |
| AUDIT_SCOPE | `docs/AUDIT_SCOPE.md` | ✅ This document |
| THIRD_PARTY_COMPONENTS | `docs/THIRD_PARTY_COMPONENTS.md` | ✅ Created |

## Sign-off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Technical Lead | | | |
| Quality Assurance | | | |
| Safety Reviewer | | | |
| Sponsor | | | |
