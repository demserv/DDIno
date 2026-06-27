<!-- @requirement AF.1, AF.2, BLOCO 01/N..13/N, Fase 0..7 -->
# Adendos Funcionais e Blocos Normativos

## AF.1 — Correção de pinagem do display
Conforme SRS Técnico Consolidado, o barramento SPI ILI9488 opera em modo 4-wire com CS dedicado (GPIO10) e DC via GPIO11.

## AF.2 — Expansão de ALM para sensores
Adicionados ALM-060 a ALM-065 para falhas de sensor ACS712 e PZEM.

## Blocos Normativos
- BLOCO 01/N: Prioridade heater > cooler: HEATER e COOLER nunca simultâneos.
- BLOCO 02/N: Lockout mínimo 500ms entre OFF→ON.
- BLOCO 03/N: SAFE_OFF exige causa resolvida + ACK.
- BLOCO 04/N: EMERGENCY sinaliza buzzer + LED até power cycle.
- BLOCO 05/N: Wizard ao primeiro boot obrigatório.
- BLOCO 06/N: Log persistente em SD com rotação.
- BLOCO 07/N: OTA com rollback.
- BLOCO 08/N: Mutex SPI para barramento compartilhado.
- BLOCO 09/N: NVS com schema versionado.
- BLOCO 10/N: Self-test obrigatório no boot.
- BLOCO 11/N: Factory reset via hardware.
- BLOCO 12/N: API REST com JWT.
- BLOCO 13/N: Web dashboard SPA.
