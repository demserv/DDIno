# Sprint Compliance 95% — Incidentes e Variâncias

Atualizado: 2026-06-27 — execução completa T-01..T-18 (build **não** validado — sem ESP-IDF neste PC).

## Resolvidas nesta sessão

- **T-02:** Drivers renomeados para `driver_ili9488.*`, `driver_xpt2046.*`, `driver_ad_keypad_lvgl.*`
- **T-10:** `conf_ctl` removido; `storage_sd.c` usa default 512 KB para rotação de log

## Variâncias residuais (aceitas)

- **T-04:** Enum `safeoff_reason_t` usa nomes internos (`SAFEOFF_REASON_THERMAL_CRITICAL`, etc.); JSON `/api/v1/state` expõe strings canônicas do SRS
- **Build:** `idf.py` indisponível — validação de compilação pendente no ambiente com ESP-IDF

## Branch e commits

Branch alvo: `sprint/compliance-95-percent`  
Commits atômicos T-XX registrados no histórico git.
