<!-- @requirement RB-REL-001..008 -->
# Regras de Relé

- RB-REL-001: Relés são active-high. OFF = nível LOW.
- RB-REL-002: Todos OFF no boot (estado seguro).
- RB-REL-003: Nenhum relé pode ser acionado antes do self-test.
- RB-REL-004: Falha de COM do relé → estado FAULT.
- RB-REL-005: Lockout impede ON imediato pós OFF.
- RB-REL-006: Heater e cooler exclusivos (verificado por software + hardware).
- RB-REL-007: Bloqueio manual persiste pós-reboot.
- RB-REL-008: EMERGENCY desliga relés independente de bloqueio.
