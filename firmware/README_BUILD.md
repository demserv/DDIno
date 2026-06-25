# Build Foundation - Monitor Aquario Inteligente (FASE 0)

## Stack e baseline
- Framework: **ESP-IDF v5.x**
- Target: **esp32s3**
- RTOS: FreeRTOS nativo do ESP-IDF
- Projeto: `monitor_aquario_inteligente_fw`

## Pre-requisitos
- ESP-IDF instalado e exportado no ambiente
- Python e toolchain conforme documentacao oficial do ESP-IDF

## Passo a passo (build limpo)
```bash
cd firmware
idf.py set-target esp32s3
idf.py fullclean
idf.py build
```

## Flash e monitor
```bash
idf.py -p <PORTA_SERIAL> flash monitor
```

## Criterios de aceite da FASE 0
- Build sem erro
- Boot executa `app_main`
- Motivo de reset registrado em log
- Reles diretos P01/P02 iniciam em OFF (nivel logico seguro)
- Barramentos I2C/SPI/UART inicializados

## Seguranca eletrica (normativa)
- Nao energizar rede AC sem checklist de seguranca aprovado
- Nao usar protoboard/Dupont para dominio AC
- Fail-safe OFF obrigatorio em reset/falha
