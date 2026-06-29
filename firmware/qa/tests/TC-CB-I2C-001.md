# TC-CB-I2C-001: Circuit Breaker I2C Recovery

## Objetivo
Verificar que o circuit breaker do barramento I2C detecta falhas, transita para OPEN apos `failure_threshold`, e recupera para HALF_OPEN apos `half_open_timeout_ms`, e que uma operacao bem-sucedida em HALF_OPEN restaura CLOSED.

## Pre-condicoes
1. Hardware real montado com dispositivos I2C (MCP23017, DS3231)
2. Firmware compilado e rodando

## Procedimento
1. Simular falhas I2C (desconectar SDA/SCL ou remover dispositivo)
2. Chamar `circuit_breaker_record_failure(CB_BUS_I2C)` repetidamente
3. Verificar que apos `failure_threshold` falhas, o estado transita para OPEN
4. Aguardar `half_open_timeout_ms`
5. Verificar que o estado transita automaticamente para HALF_OPEN
6. Reaplicar sinal I2C e chamar `circuit_breaker_record_success(CB_BUS_I2C)` dentro da janela de teste
7. Verificar que o estado retorna a CLOSED
8. Verificar que `circuit_breaker_is_available(CB_BUS_I2C)` retorna false em OPEN e true em CLOSED/HALF_OPEN

## Criterios de Aceite
- CLOSED → OPEN apos `failure_threshold` falhas consecutivas
- OPEN → HALF_OPEN automaticamente apos `half_open_timeout_ms`
- HALF_OPEN → CLOSED apos 1 sucesso
- `is_available()` retorna false em OPEN

## Evidencia
- Logs de transicao de estado do circuit breaker
- Medicoes de tempo das transicoes

## Status
NOT_TESTED
