# TC-HW-RELAY-5V-IF-001: Interface de Rele 5V

## Objetivo
Verificar que o modulo de reles externo (8 canais, 5V, optoacoplado, MCP23017 via I2C) opera corretamente nos niveis de tensao e logica especificados.

## Pre-condicoes
1. Hardware real montado (ESP32-S3 + MCP23017 + modulo de reles 5V)
2. Fonte 5V dedicada para o modulo de reles
3. Jumpers JD-VCC configurados para opto-isolamento
4. Firmware compilado e rodando

## Procedimento
1. Inicializar o driver: `relay_init_safe()`
2. Verificar que todos os reles estao OFF apos init (medir tensao nos contatos)
3. Ligar cada rele individualmente via `relay_on(plug_id)`:
   - P03..P10 (MCP23017): Verificar que o rele correspondente fecha
   - P01..P02 (GPIO direto): Verificar que o rele correspondente fecha
4. Desligar cada rele via `relay_off(plug_id)`: Verificar que abre
5. Executar `relay_all_off()`: Verificar que todos os 10 reles abrem
6. Medir tensao VCC no modulo: deve estar entre 4.75V e 5.25V
7. Verificar que o LED indicador de cada rele acende quando ON

## Criterios de Aceite
- Todos os 10 reles alternam ON/OFF corretamente
- `relay_all_off()` desliga todos os reles simultaneamente
- Tensao do modulo dentro de 4.75V-5.25V
- Nenhum rele permanece ON apos `relay_init_safe()`

## Evidencia
- Medicoes de tensao registradas
- Logs seriais com estado de cada rele
- Checklist AC_SAFETY_CHECKLIST.md itens 6, 10, 11 confirmados

## Status
NOT_TESTED
