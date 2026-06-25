# Arquitetura do Firmware — Monitor de Aquário Inteligente (DDIno)

## 1. Visão Geral

Firmware bare-metal/RTOS para ESP32-S3 (N16R8) que implementa um monitor/controlador de aquário com:

- 10 tomadas inteligentes (relés via MCP23017 I2C)
- Sensores: temperatura (DS18B20), nível ATO (XKC-Y25-V via ADC), corrente (ACS712 20A × 8 + ADC), energia (PZEM-004T v3.0)
- RTC DS3231 para timestamp
- Display TFT 480×320 + Touch XPT2046 + AD Keypad
- API HTTP `/api/v1` para integração LAN
- Safety pipeline com 3 FSMs + Safety Controller + Circuit Breaker + WDT

## 2. Mapa de Módulos

```
app_main.c  ← entry point (loop 50ms)
  │
  ├── hal/          inicialização I2C, SPI, UART
  ├── drivers/      acesso direto ao hardware
  │   ├── driver_mcp23017    I2C: relés P03-P10, buzzer, LED
  │   ├── driver_relay       API uniforme P01-P10 (P01,P02 GPIO; P03-P10 MCP)
  │   ├── driver_mcp3208     SPI: 2× MCP3208 (ADC corrente, ATO, keypad)
  │   ├── driver_acs712      leitura corrente por plug
  │   ├── driver_ad_keypad   decodificação teclado 5V condicionado
  │   ├── driver_pzem        UART: PZEM-004T v3.0 (V, A, W, Wh, Hz, PF)
  │   ├── driver_ds18b20     1-Wire bit-banged (GPIO4)
  │   ├── driver_ds3231      I2C: RTC DS3231
  │   └── driver_buzzer_led  LED 3 cores + buzzer via MCP23017 GPB
  ├── core/          lógica central de safety
  │   ├── safety_controller  máquina de estados global (NORMAL→DEGRADED→SAFE_OFF→EMERGENCY)
  │   └── circuit_breaker    padrão circuit breaker para cada barramento
  ├── fsm/           máquinas de estado especializadas
  │   ├── thermal_fsm    temperatura (NORMAL/ALERT/CRITICAL/EXTREME/SENSOR_FAIL)
  │   └── ato_fsm        nível ATO (NORMAL/REFILLING/OVERFLOW/BLOCKED/ERROR/DISABLED)
  ├── services/      serviços de aplicação
  │   ├── command_validator   valida se um comando é permitido dado o estado global
  │   ├── relay_safety_service bloqueia acionamento se safety violado
  │   ├── alert_manager       gerenciamento de alarmes (ALM-001 a ALM-065)
  │   ├── self_test           autoteste de HW na inicialização
  │   ├── electric_fsm        FSM elétrica (NORMAL/HIGH_CONSUMPTION/OVERLOAD/SENSOR_FAIL)
  │   ├── safeoff_alm_map     mapeia safeoff_reason → ALM-ID
  │   ├── cdn_energy          acumulador de energia Wh por plugue
  │   ├── storage_sd          log + backup em cartão SD (FATFS/SPI)
  │   └── wdt_advanced        watchdog configurável por tarefa
  ├── web/           servidor HTTP
  │   ├── api_rest         handlers + roteamento /api/v1/*
  │   ├── api_auth         autenticação SHA-256 + token RAM
  │   └── api_rate_limit   rate limit por IP (30 req/min)
  ├── ui/            interface LVGL
  │   ├── ui_display      init display ST7789/ILI9341 via SPI
  │   ├── ui_touch        touch XPT2046 SPI
  │   ├── ui_keypad       teclado analógico como LVGL indev
  │   ├── ui_screens      gerenciamento carrossel (7 telas)
  │   └── screens/        telas individuais (dashboard, devices, energy, ...)
  └── include/       headers canônicos compartilhados
      ├── pin_map.h           mapeamento GPIO
      ├── hardware_config.h   constantes de HW
      ├── alm_ids.h           enum ALM-001 a ALM-065
      ├── system_types.h      tipos base (system_state_t, safeoff_reason_t, ...)
      ├── global_state.h      struct do estado global
      └── ...
```

## 3. Sequência de Boot

```
app_main()
  │
  ├─ init_nvs_safe()              recuperação de páginas NVS
  ├─ init_global_state()          g_gs = { SYSTEM_STATE_NORMAL, ... }
  ├─ safety_controller_init()    safeoff_reason = NONE
  ├─ alert_manager_init()        slots limpos
  ├─ self_test_init()            prepara tabela de testes
  ├─ hal_bus_init_all()          I2C(0), SPI(2), UART(PZEM)
  ├─ circuit_breaker_init()      todos CLOSED
  ├─ relay_init_safe()           todos relés OFF
  ├─ mcp3208_init() + acs712 + ad_keypad + pzem + ds18b20 + ds3231 + buzzer
  ├─ cdn_energy_init()           acumuladores zerados
  ├─ storage_sd_init()           monta FATFS SPI (falha não bloqueante)
  ├─ thermal_fsm / ato_fsm / electric_fsm init
  ├─ ui_display_init() + ui_screens_init()
  ├─ api_rest_init()             servidor HTTP porta 80
  ├─ wdt_advanced_init() + register(MAIN_LOOP, 2000ms)
  └─ self_test_run_all(10000ms)  testa HW, loga resultados
        ↓
   loop principal 50ms (while(1))
```

## 4. Loop Principal (50 ms)

Cada iteração executa na ordem:

```
1. circuit_breaker_update()        → OPEN→HALF_OPEN se timeout expirou
2. wdt_advanced_reset(MAIN_LOOP)   → pet no WDT
3. read_temp()                     → DS18B20 via circuit breaker
4. read_ato_level()                → ADC via circuit breaker
5. thermal_fsm_update()            → calcula estado térmico
6. ato_fsm_update()                → calcula estado ATO
7. acs712_read_plug() × 10        → correntes via circuit breaker
8. pzem_read_all()                 → energia via circuit breaker
9. electric_fsm_update()           → calcula estado elétrico
10. safety_controller_evaluate()   → determina estado global
11. alert_manager_raise()          → alarme se SAFE_OFF
12. relay_all_off() se SAFE_OFF/EMERGENCY
13. cdn_energy_update() × 10      → acumula Wh
14. led_set()                      → LED conforme estado
15. ui_screen_update_all() + lv_timer_handler()
16. vTaskDelay(50ms)
```

## 5. Safety Pipeline

```
Sensores → FSMs → Safety Controller → Global State → Relay Enforcement
   │          │            │                │               │
   │    thermal_fsm   force_emergency   SYSTEM_STATE_   relay_all_off()
   │    ato_fsm       force_safe_off    EMERGENCY       se >= SAFE_OFF
   │    electric_fsm   degraded         SAFE_OFF
   │                                    DEGRADED
   │                                    NORMAL
   ▼
Circuit Breaker (por barramento)
   CLOSED → falhas >= 5 → OPEN → 30s → HALF_OPEN → 3 sucessos → CLOSED
```

**Prioridade**: `EMERGENCY > SAFE_OFF > DEGRADED > NORMAL`

Cada FSM publica flags `force_emergency`, `force_safe_off`, `degraded_condition`.
O Safety Controller usa a flag de maior prioridade como estado resultante.

## 6. Circuit Breaker

| Bus ID | Barramento | Threshold | Half-open | Monitorado por |
|--------|-----------|-----------|-----------|----------------|
| I2C | I2C-0 | 5 falhas | 30s | hal_bus (MCP23017, DS3231) |
| SPI_ADC | SPI2 MCP3208 | 5 | 30s | read_ato_level(), ACS712 reads |
| SPI_DISPLAY | SPI2 Display | 5 | 30s | UI LVGL |
| SPI_SD | SPI2 SD Card | 5 | 30s | storage_sd |
| UART_PZEM | UART PZEM | 5 | 30s | pzem_read_all() |
| DS18B20 | 1-Wire GPIO4 | 5 | 30s | read_temp() |

## 7. Máquinas de Estado

### Thermal FSM
```
        ┌──────────┐
        │  NORMAL  │ ← t dentro da faixa (sp ± h)
        └────┬─────┘
     t > sp+h │ t < sp-h
        ┌────▼─────┐
        │  ALERT   │ → request_cooler_on ou request_heater_on
        └────┬─────┘
  t >= critical │ t >= extreme (se enabled)
        ┌───────▼──────┐        ┌──────────┐
        │  CRITICAL    │        │ EXTREME  │ → force_emergency
        │ force_safe_off│        └──────────┘
        └──────────────┘
        ┌──────────────┐
        │ SENSOR_FAIL  │ ← sample_valid == false
        └──────────────┘
```

### ATO FSM
```
        ┌──────────┐
        │  NORMAL  │ ← level ≥ low
        └────┬─────┘
      level < low
        ┌───────────┐
        │ REFILLING │ → pump_request_on, timer countdown
        └─────┬─────┘
  timeout 120s │   level ≥ low
        ┌──────▼─────┐          ┌──────────┐
        │  BLOCKED   │          │  NORMAL  │
        │ pump OFF   │          └──────────┘
        └────────────┘
   level > overflow
        ┌──────────┐
        │ OVERFLOW │ → force_safe_off, blocked_latched
        └──────────┘
        ┌──────────┐
        │  ERROR   │ ← sample_valid == false
        └──────────┘
        ┌──────────┐
        │ DISABLED │ ← cfg.enabled == false
        └──────────┘
```

### Electric FSM
```
        ┌──────────┐
        │  NORMAL  │ ← potência total < limite - histerese
        └────┬─────┘
   potência na faixa de alerta
        ┌──────────────────┐
        │ HIGH_CONSUMPTION │ ← entre (limite - h) e limite
        └────────┬─────────┘
  > limite total │ > corrente por plug
        ┌──────────▼──────┐
        │   OVERLOAD      │ → force_safe_off
        │ (total ou plug) │
        └─────────────────┘
        ┌──────────┐
        │SENSOR_FAIL│ ← sample_valid == false
        └──────────┘
```

## 8. Estados Globais

```
NORMAL    → operação normal, todas as funções habilitadas
DEGRADED  → falha não crítica (ex: SD offline, display offline), operação continua
SAFE_OFF  → condição de safety detectada, todos relés desligados, monitora apenas
EMERGENCY → temperatura extrema, emergência incondicional
```

Transições: `NORMAL ↔ DEGRADED`, `qualquer → SAFE_OFF`, `qualquer → EMERGENCY`
Recuperação de SAFE_OFF/EMERGENCY exige wizard ou reset.

## 9. Pilha Web API

```
httpd (esp_http_server porta 80)
  └── auth_guard middleware
       ├── is_auth_path? → libera /login e /logout
       ├── auth_middleware  → valida X-Auth-Token (SHA-256 + RAM)
       └── rate_limit_middleware → 30 req/min/IP

Rotas:
  POST /api/v1/auth/login    → {user, password} → token (3600s)
  POST /api/v1/auth/logout   → revoga token
  GET  /api/v1/status        → estado global + alarmes
  GET  /api/v1/plugs         → estado + corrente de cada plug
  POST /api/v1/plugs         → {id, state} via safety pipeline
  GET  /api/v1/sensors       → temperatura, ATO, keypad, RTC
  GET  /api/v1/energy        → V, A, W, Wh, Hz, PF (PZEM)
  GET  /api/v1/alerts        → alarmes ativos
  POST /api/v1/alerts        → acknowledge todos alarmes
  GET  /api/v1/config        → wizard_completed, monitor_only_mode
  POST /api/v1/config        → altera configuração
  POST /api/v1/command       → {cmd, plug_id, state} via command_validator
  GET  /api/v1/log           → últimos eventos
```

## 10. Pilha UI/HMI (LVGL v8.x)

```
lvgl (8.3)
  ├── display: ST7789/ILI9341 480×320 via SPI (CS, DC, RST, BL)
  ├── touch:  XPT2046 SPI (CS, IRQ) → LV_INDEV_TYPE_POINTER
  ├── keypad: AD Keypad via MCP3208 CH3 → LV_INDEV_TYPE_KEYPAD
  ├── state_badge: lv_layer_top() com texto + cor (NORMAL✓/DEGRADED⚠/SAFE_OFF⛔/EMERGENCY🚨)
  └── carrossel 7 telas:
      0. Dashboard      → temperatura, tomadas on, alarmes ativos, energia total
      1. Devices 1/2    → P01-P05, P06-P10 (toggle ON/OFF + corrente)
      2. Energy         → V, A, W, Wh, Hz, PF
      3. Alerts         → lista scrollável + botão ACK
      4. Menu           → Config, Alarmes, Feed, Manutenção, Wizard, Sobre
      5. Submenu        → tensão 127/220, temperatura alvo, potência máxima
```

Navegação: touch/teclas avança/volta, timeout 60s volta ao Dashboard.

## 11. Armazenamento

| Dados | Local | Fatecimento |
|-------|-------|-------------|
| Configuração (wizard, modo) | NVS | RAM fallback |
| Alarmes ativos | RAM (`alert_slot_t[80]`) | Perde ao reset |
| Logs estruturados | SD FATFS (`/sdcard/{events,alerts,energy,audit}/log.txt`) | Opcional |
| Backup de config | SD FATFS (`/sdcard/config_backup.json`) | Opcional |
| Acumuladores CDN | RAM (`cdn_entry_t[10]`) | Inicia zerado |

## 12. Mapeamento de Pinos

| GPIO | Função | Periférico |
|------|--------|------------|
| 4 | DQ | DS18B20 (1-Wire) |
| 5 | OUT | Relé P01 |
| 6 | OUT | Relé P02 |
| 8 | SDA | I2C-0 |
| 9 | SCL | I2C-0 |
| 10 | CS | TFT |
| 11 | MOSI | SPI2 |
| 12 | MISO | SPI2 |
| 13 | SCK | SPI2 |
| 15 | CS | SD Card |
| 16 | DC | TFT |
| 17 | TX | PZEM UART |
| 18 | RX | PZEM UART |
| 21 | RST | TFT |
| 38 | CS | Touch XPT2046 |
| 39 | IRQ | Touch XPT2046 |
| 41 | CS | MCP3208 #1 (ACS712 P01-P08) |
| 42 | CS | MCP3208 #2 (P09, P10, ATO, Keypad) |
| 47 | BL | TFT Backlight |

**MCP23017 (I2C 0x20)**:
- GPA0-7 → relés P03-P10
- GPB2 → LED Verde, GPB3 → LED Amarelo, GPB4 → LED Vermelho, GPB5 → Buzzer

**MCP3208 #1 (CS=41)** CH0-7: ACS712 P01-P08
**MCP3208 #2 (CS=42)** CH0: P09, CH1: P10, CH2: ATO XKC-Y25-V, CH3: AD Keypad

## 13. Estratégia de Watchdog

- `esp_task_wdt_init(10, true)`: WDT global 10s com panic
- `wdt_advanced` registra `main_loop` com reset a cada iteração (50ms)
- Em caso de hung > 10s, WDT força reset do SoC
- Tarefas UI e Web podem ser registradas no WDT individualmente

## 14. Tratamento de Falhas

| Falha | Ação | Recuperação |
|-------|------|-------------|
| I2C MCP23017 sem resposta | Circuit breaker OPEN + alerta | HALF_OPEN após 30s |
| SPI ADC sem resposta | Circuit breaker OPEN, ACS712/ATO indisponíveis | HALF_OPEN após 30s |
| SD falha | Sistema continua sem storage | Não recupera |
| PZEM sem resposta | Circuit breaker OPEN, energia não atualizada | HALF_OPEN após 30s |
| DS18B20 sem resposta | Circuit breaker OPEN, temperatura default 25°C | HALF_OPEN após 30s |
| Temperatura crítica | SAFE_OFF + alarme ALM-026 | Reset manual |
| NVS corrompido | NVS erase + recovery | Automático |
| WDT timeout | Reset do SoC | POST + self-test |
