<!-- @requirement RB-TERM-001..024 -->
# Terminologia Canônica — Monitor Aquário Inteligente

- **ATO**: Auto Top-Off. Sistema de reposição automática de água.
- **SAFE_OFF**: Estado seguro com todos os relés desligados. Requer causa resolvida + ACK para sair.
- **EMERGENCY**: Estado crítico com alarme sonoro contínuo. Apenas reset físico ou power cycle saem.
- **Plug**: Cada saída de relé individual (P01..P10).
- **Role**: Função atribuída a um plug (heater, cooler, light, filter, pump, feeder).
- **Lockout**: Tempo mínimo entre OFF→ON de um relé (anti-chatter).
- **Wizard**: Sequência de configuração inicial pós-flash.
- **FSM**: Máquina de estados finitos.
- **HAL**: Hardware Abstraction Layer.
- **ALM**: Código de alarme/alerta.
- **NVS**: Non-Volatile Storage (partição flash ESP32).
- **SPIFFS**: SPI Flash File System (partição storage/logs).
- **CDN Energy**: Contador de energia por plug.
- **Observation Mode**: Modo monitor-only sem atuação.
- **Maintenance Mode**: Modo de manutenção com override manual.
