<!-- @requirement RB-EST-001..010 -->
# Regras de Estado de Plugue

- RB-EST-001: Plug inicia OFF no boot.
- RB-EST-002: Plug só ON se global_state == NORMAL.
- RB-EST-003: Plug HEATER bloqueia se COOLER ON (inversa também).
- RB-EST-004: Lockout mínimo 500ms após OFF.
- RB-EST-005: SAFE_OFF → todos plugs OFF imediatamente.
- RB-EST-006: P02 (heater) e P03 (cooler) têm role fixa.
- RB-EST-007: Bloqueio manual sobrescreve automático.
- RB-EST-008: Bloqueio persiste via NVS.
- RB-EST-009: Religamento automático respeita lockout.
- RB-EST-010: Cada plug tem estado: OFF, ON, BLOCKED, FAULT.
