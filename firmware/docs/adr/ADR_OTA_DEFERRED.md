# ADR-OTA-DEFERRED: OTA via Application Firmware Deferida

## Status
ACCEPTED (2026-06-28)

## Context
O SRS v3.11 (RF-HW-OTA-001) especifica suporte a OTA (Over-The-Air) para atualizacao de firmware. No entanto, a implementacao de OTA confiavel dentro da aplicacao apresenta riscos:

1. **Complexidade de rollback**: Gerenciar fallback por software em caso de falha de boot apos OTA.
2. **Uso de flash**: Particao OTA consome flash que pode ser melhor utilizado para storage (NVS / SPIFFS).
3. **Seguranca**: Validacao de imagem OTA e protecao contra brick requer assinatura e verificacao robusta.
4. **Integracao com bootloader**: O ESP-IDF oferece suporte nativo a OTA via `esp_https_ota`, mas a integracao segura requer servidor dedicado e certificados.

## Decision
OTA via application firmware foi **DEFERIDA** para uma versao futura (v2.0.0). A versao atual (F1.0.0) nao implementa OTA. Atualizacoes de firmware serao feitas via:

- **USB/UART**: Atraves do bootloader do ESP32-S3 (`esptool.py` / `idf.py flash`).
- **Conexao serial dedicada**: Pino de boot (GPIO0) acessivel no conector de programacao.

## Consequences

### Positive
- Reducao do risco de brick durante atualizacao remota.
- Mais flash disponivel para armazenamento local (NVS + SPIFFS).
- Menos codigo na aplicacao para gerenciamento de particoes OTA.

### Negative
- Atualizacoes requerem acesso fisico ao dispositivo ou conexao serial.
- Nao e possivel corrigir bugs de firmware remotamente.
- Requer planejamento para implementacao futura com servidor OTA dedicado e assinatura de imagem.

## Compliance Note
O requisito RF-HW-OTA-001 esta marcado como **DEFERRED** no RTM. A implementacao sera reavaliada na proxima revisao major (v2.0.0).
