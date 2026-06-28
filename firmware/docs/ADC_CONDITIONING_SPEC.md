# ADC_CONDITIONING_SPEC

| Requisito | RF-HW-ADC-* / AF.3 |
|-----------|-------------------|

## MCP3208 (SPI)

| Canal | Função | CS |
|-------|--------|-----|
| #1 | ACS712 P01–P05 (condicionado) | GPIO41 |
| #2 CH2 | ATO digital ON/OFF | GPIO42 |

- Clock SPI: 1 MHz (`HW_SPI_CLK_MCP3208_HZ`)
- Faixa contagem: 0–4095 (`HW_ADC_MAX_COUNT`)
- Vref lógica: 3.3 V (`HW_ADC_VREF_MV`)

## ATO digital (RF-ATO-DIGITAL-001)

- Entrada condicionada 0–3.3 V
- LOW/HIGH/overflow configuráveis via Wizard (`low_level_adc`, `high_level_adc`, `overflow_margin_adc`)
- Debounce: 3 amostras (`ato_fsm.c`)

## ACS712 20A (RF-PLUG-003)

- Alimentação sensor: 5 V (`HW_ACS712_SUPPLY_MV`)
- Saída condicionada para 0–3.3 V (`HW_ACS712_CONDITIONED_TO_3V3`)
- Offset/gain calibrável por canal NVS
- RMS obrigatório (`driver_acs712.c`)

## Keypad AD (RF-UI-INPUT-*)

- Alimentação 5 V, entrada condicionada ADC (`HW_AD_KEYPAD_ADC_CONDITIONED`)

Implementação: `drivers/driver_mcp3208.c`, `drivers/driver_acs712.c`, `drivers/driver_ad_keypad.c`
