# TC-HW-ADC-RANGE-001: Faixa do ADC MCP3208 (Nivel ATO)

## Objetivo
Verificar que o ADC MCP3208 nos canais de nivel ATO retorna valores dentro da faixa especificada (0-4095) e que a resolucao de 12 bits e alcancada.

## Pre-condicoes
1. Hardware real montado (ESP32-S3 + MCP3208 + sensor de nivel ATO conectado ao canal CH0)
2. Firmware compilado e rodando
3. `circuit_breaker_is_available(CB_BUS_SPI_ADC)` retorna true

## Procedimento
1. Conectar o pino de sinal do sensor ATO ao GND (simulando nivel 0)
2. Executar `mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &adc_val)`
3. Verificar que `adc_val` esta entre 0 e 10
4. Conectar o pino de sinal a VREF (3.3V, simulando nivel maximo)
5. Repetir a leitura e verificar que `adc_val` esta entre 4085 e 4095
6. Executar 100 leituras consecutivas; verificar que todas estao na faixa 0-4095
7. Verificar que `circuit_breaker_record_success/failure` e chamado corretamente

## Criterios de Aceite
- Valor minimo ~0 (GND) ≤ 10
- Valor maximo ~4095 (VREF) ≥ 4085
- Todas as 100 leituras consecutivas na faixa 0-4095
- Nenhuma falha do circuit breaker durante o teste

## Evidencia
- Logs seriais com valores lidos
- Captura de tela do monitor serial

## Status
NOT_TESTED
