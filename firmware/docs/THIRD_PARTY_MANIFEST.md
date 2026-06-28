# Third-Party Manifest — Monitor Aquário Inteligente

## Version

| Field | Value |
|-------|-------|
| Document Version | 1.0 |
| Date | 2026-06-27 |
| Firmware Version | compliance-fix-v1 |
| SRS Baseline | Técnico Consolidado Final v3.11-AF.3 + AF.4 + Bloco 12/N + Bloco 13/N |

## Purpose

Formal manifest of all third-party components included in the firmware deliverable. This document supports audit traceability by separating project-authored code from vendor/community code.

## Component Table

| # | Component | Version | License | Source | Integration | In Scope? |
|---|-----------|---------|---------|--------|-------------|-----------|
| 1 | LVGL | ^8.3 (pinned) | MIT | `lvgl/lvgl` via IDF component manager | `managed_components/lvgl__lvgl/` | No — vendor |
| 2 | ESP-IDF | >=4.1.0 (v5.x expected) | Apache 2.0 | Espressif | System toolchain + framework | No — vendor |
| 3 | FreeRTOS | (included in ESP-IDF) | MIT | Amazon/Espressif | Kernel via ESP-IDF | No — vendor |
| 4 | Unity | (in ESP-IDF test) | MIT | ThrowTheSwitch | Test runner | No — test harness |
| 5 | CMock | (in ESP-IDF test) | MIT | ThrowTheSwitch | Mock generator | No — test harness |

## Integrity Verification

- All components used as-is from official sources; no local patches or forks.
- LVGL: verified via `idf_component.yml` hash / version pin.
- ESP-IDF: standard Espressif toolchain; environment variable `IDF_PATH` defines the baseline.
- No proprietary or binary-only third-party components are linked.

## Exclusion Rationale

Third-party code is excluded from audit scope because:
1. It is not authored by the project team and cannot be altered.
2. It is pinned by reference (`idf_component.yml`, system toolchain) and not subject to project SRS requirements.
3. Including vendor code in compliance concatenation would inflate scope with non-project code and obscure the audit trail.

Refer to `AUDIT_SCOPE.md` for detailed boundary definitions.
