# TC-CB-UART-PZEM-001: Circuit Breaker UART PZEM Recovery

## Objetivo
Verificar que o circuit breaker do barramento UART (PZEM-004T v4.0) detecta falhas de comunicacao, isola o barramento, e recupera apos restauracao da comunicacao.

## Pre-condicoes
1. Hardware real montado (ESP32-S3 + PZEM-004T v4.0 via UART)
2. Firmware compilado e rodando

## Procedimento
1. Verificar que `circuit_breaker_is_available(CB_BUS_UART_PZEM)` retorna true
2. Executar `pzem_read_all(&g_pzem)` com sucesso → CB registra sucesso
3. Desconectar TX/RX do PZEM (simular falha de barramento)
4. Executar `pzem_read_all(&g_pzem)` → falha → CB registra falha
5. Repetir falhas ate `failure_threshold` → CB transita para OPEN
6. Verificar que o sistema continua operando sem PZEM (leitura eletrica indisponivel, sistema em DEGRADED)
7. Reconectar TX/RX do PZEM
8. Aguardar `half_open_timeout_ms` → CB transita para HALF_OPEN
9. Executar `pzem_read_all(&g_pzem)` com sucesso → CB transita para CLOSED
10. Verificar que a leitura eletrica e restaurada

## Criterios de Aceite
- Falha de comunicacao detectada e isolada
- Sistema DEGRADED sem PZEM (sem SAFE_OFF apenas por perda de PZEM)
- Recuperacao automatica apos restauracao da comunicacao

## Evidencia
- Logs de transicao do circuit breaker
- Logs de estado do sistema (DEGRADED)

## Status
NOT_TESTED
