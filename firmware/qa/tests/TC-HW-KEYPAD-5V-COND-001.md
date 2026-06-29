# TC-HW-KEYPAD-5V-COND-001: Condicionamento do Sinal do Keypad

## Objetivo
Verificar que o ADC do keypad (GPIO41/GPIO42 via adc1) retorna valores dentro da faixa esperada para cada tecla e que o condicionamento de sinal (debounce + limiar) funciona corretamente.

## Pre-condicoes
1. Hardware real montado (ESP32-S3 + teclado resistivo + circuito de condicionamento)
2. Firmware compilado e rodando
3. `ad_keypad_init()` executado com sucesso

## Procedimento
1. Nenhuma tecla pressionada: Verificar que `ad_keypad_read()` retorna KEY_NONE
2. Pressionar cada tecla individualmente por pelo menos 100ms:
   - Verificar que o codigo da tecla corresponde ao esperado
   - Verificar que o ADC retorna valor dentro da faixa definida para cada tecla
3. Pressionar e soltar rapidamente (30ms): Verificar que o debounce rejeita
4. Manter tecla pressionada por 5s: Verificar que retorna repetidamente (auto-repeat)
5. Pressionar duas teclas simultaneamente: Verificar que retorna KEY_NONE ou a tecla de maior prioridade

## Criterios de Aceite
- KEY_NONE quando nenhuma tecla pressionada
- Cada tecla reconhecida individualmente
- Debounce rejeita pulsos < 50ms
- Auto-repeat funcional apos ~1s de pressionamento continuo

## Evidencia
- Logs seriais com valores de ADC e codigos de tecla
- Tabela de faixas ADC por tecla registrada

## Status
NOT_TESTED
