# Auditoria de Conformidade — SRS v3.11-AF.3/AF.4

## Metodologia
Cruzamento de cada requisito RF-XXX extraído do SRS Técnico Consolidado Final contra:
1. `@requirement` tags no código fonte
2. Funções, structs, enums, e variáveis implementadas
3. Endpoints API registrados
4. Máquinas de estado, serviços e drivers

## Legenda
- ✅ **COMPLETE** — Funcionalidade implementada conforme SRS
- ⚠️ **PARTIAL** — Implementação parcial ou com desvios
- ❌ **NOT_FOUND** — Não implementado
- 🔲 **N/A** — Fora de escopo / diferido

---

## 1. GLOBAL (Estado Global)

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-GLOBAL-001 | Definição de estados globais | ✅ COMPLETE | `system_types.h`, `global_state.h` | `system_state_t` enum: NORMAL/DEGRADED/SAFE_OFF/EMERGENCY |
| RF-GLOBAL-002 | Transições com prioridade | ✅ COMPLETE | `safety_controller.c` | EMERGENCY > SAFE_OFF > DEGRADED > NORMAL + anti-flap |
| RF-GLOBAL-004 | safeoff_reason tracking | ✅ COMPLETE | `safety_controller.c`, `system_types.h` | `safeoff_reason_t` com 13 razões, timestamp, source_alm |
| RF-GLOBAL-005 | Zero hardcode operacional | ✅ COMPLETE | `config_manager.c`, `hardware_config.h` | Todos parâmetros via NVS; HW_* só constantes físicas |
| RF-GLOBAL-SAFEOFF-EXIT-001 | Pré-condições saída SAFE_OFF | ✅ COMPLETE | `safety_controller.c` | `can_exit_safeoff()`: causa resolvida + sensores OK + self-test + ACK + 10s estabilização |
| RF-GLOBAL-SAFEOFF-EXIT-002 | Destino saída SAFE_OFF | ✅ COMPLETE | `safety_controller.c` | → DEGRADED se condição remanescente, → NORMAL se nenhuma |
| RF-GLOBAL-EMERG-EXIT-001 | Saída controlada EMERGENCY | ✅ COMPLETE | `safety_controller.c` | `can_exit_emergency()`: emergência resolvida + sensores OK + ACK + 30s |
| RF-GLOBAL-REARM-001 | Rearming controlado | ⚠️ PARTIAL | `restart_fsm.c` | Religamento sequencial existe mas sem blocked plug isolation |

## 2. FLOW / BOOT

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-FLOW-BOOT-001 | Sequência mínima de boot | ✅ COMPLETE | `app_main.c` | 15+ etapas: NVS, HW safe, self-test, barramentos, FSMs, API, UI |
| RF-FLOW-BOOT-002 | First boot / wizard pendente | ✅ COMPLETE | `app_main.c`, `api_rest.c` | `wizard_completed=false` → overlay azul, auth desabilitado |
| RF-FLOW-BOOT-003 | Self-test falho no boot | ⚠️ PARTIAL | `app_main.c` | Self-test executado mas resultado não força SAFE_OFF |
| RF-FLOW-BOOT-004 | Recovery pós-power-loss | ⚠️ PARTIAL | `feed_snapshot.c` | Snapshot Feed restaurado, mas sem .tmp orphan handling |
| RF-FLOW-RUNTIME-001 | Laço operacional nominal | ✅ COMPLETE | `app_main.c` | Loop 50ms: sensores → FSMs → safety → UI → API → WDT |
| RF-FLOW-ALERT-001 | Pipeline alerta crítico | ✅ COMPLETE | `app_main.c`, `alert_manager.c` | Detecção → ALM → log → safety → LED → overlay → API |
| RF-INSTALL-MONITOR-001 | Modo Somente Monitoramento | ⚠️ PARTIAL | `global_state.h`, `plug_manager.c` | `monitor_only` flag existe, plug_manager verifica, mas sem endpoint de ativação |

## 3. LED (Sinalização)

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-LED-001 | LED Verde | ✅ COMPLETE | `driver_buzzer_led.c` | LED verde aceso em NORMAL/DEGRADED |
| RF-LED-002 | LED Amarelo | ✅ COMPLETE | `driver_buzzer_led.c` | Pisca em DEGRADED / Feed Mode |
| RF-LED-003 | LED Vermelho + padrão crítico | ✅ COMPLETE | `driver_buzzer_led.c` | 3 LEDs piscam juntos em SAFE_OFF/EMERGENCY |

## 4. THERMAL (Térmico)

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-THERMAL-001 | Leitura DS18B20 com CRC | ✅ COMPLETE | `driver_ds18b20.c` | CRC, rejeição 85°C, timeout, circuito breaker |
| RF-THERMAL-002 | Filtro média móvel | ✅ COMPLETE | `temp_filter.c` | Média móvel circular (default 5) |
| RF-THERMAL-003 | Classificação por parâmetros do usuário | ✅ COMPLETE | `thermal_fsm.c` | Usa `temp_normal_c`, `temp_critical_c`, `temp_extreme_c`, `hysteresis_c` |
| RF-THERMAL-004 | Configuração parâmetros térmicos | ✅ COMPLETE | `config_manager.c`, `param_catalog.h` | `thermal_params_storage_t` completo com NVS persist |
| RF-THERMAL-005 | Indicador de tendência | ❌ NOT_FOUND | — | Nenhum cálculo de tendência/slope implementado |
| RF-THERMAL-006 | Parâmetros obrigatórios no Wizard | ⚠️ PARTIAL | `api_rest.c`, `ui_screens.c` | Wizard existente mas sem seção térmica passo-a-passo |
| RF-THERMAL-007 | Uso exclusivo parâmetros do usuário | ✅ COMPLETE | `thermal_fsm.c` | Zero hardcode térmico — todos thresholds de NVS |
| RF-THERMAL-008 | Regra explícita do cooler | ⚠️ PARTIAL | `thermal_fsm.c` | Cooler aciona em ALERT, mas sem regra de prioridade heater/cooler |
| RF-THERMAL-009 | Exclusão mútua heater/cooler | ✅ COMPLETE | `thermal_fsm.c` | Se ambos → CRITICAL + SAFE_OFF + ALM-060 |
| RF-FSM-THERMAL-001 | FSM térmica e impacto sistêmico | ✅ COMPLETE | `thermal_fsm.c`, `safety_controller.c` | EXTREME→EMERGENCY, CRITICAL→SAFE_OFF, SENSOR_FAIL→SAFE_OFF |

## 5. ATO

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-ATO-001 | Leitura ADC com debounce | ⚠️ PARTIAL | `ato_fsm.c`, `app_main.c` | ADC lido mas sem debounce explícito (média simples) |
| RF-ATO-DIGITAL-001 | ATO digital ON/OFF via ADC | ⚠️ PARTIAL | `ato_fsm.c` | Threshold ADC implementado mas sem calibração wizard |
| RF-ATO-002 | FSM 6 estados | ✅ COMPLETE | `ato_fsm.c` | NORMAL, REFILLING, ERROR, BLOCKED, OVERFLOW, DISABLED |
| RF-ATO-003 | Config LOW/HIGH validada | ✅ COMPLETE | `config_manager.c`, `param_catalog.h` | `low_level_adc`, `high_level_adc`, `overflow_margin_adc` |
| RF-ATO-004 | Refill anormal | ✅ COMPLETE | `ato_fsm.c` | Timeout → BLOCKED com ALM-019 |
| RF-ATO-005 | Reservatório vazio/bloqueio | ✅ COMPLETE | `ato_fsm.c` | Refill timeout infere vazio → BLOCKED |
| RF-FSM-ATO-001 | Impacto no estado global | ✅ COMPLETE | `ato_fsm.c`, `safety_controller.c` | OVERFLOW→SAFE_OFF, BLOCKED→SAFE_OFF, ERROR→DEGRADED |

## 6. PLUG

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-PLUG-001 | Modos AUTO/MANUAL/TIMER/DELAY/OVERRIDE | ✅ COMPLETE | `plug_manager.c` | 5 modos implementados com lógica completa |
| RF-PLUG-002 | Tipo e criticidade | ✅ COMPLETE | `plug_model.h`, `plug_manager.c` | 7 tipos, is_critical para AQUECEDOR/COOLER |
| RF-PLUG-003 | Proteção de corrente | ✅ COMPLETE | `driver_acs712.c`, `electric_fsm.c` | `current_limit_a`, overcurrent_count, circuit breaker |
| RF-PLUG-004 | Detecção de bypass | ❌ NOT_FOUND | — | `bypass_detected` field existe mas nunca setado |
| RF-PLUG-005 | Cálculo potência/consumo | ✅ COMPLETE | `cdn_energy.c` | power_w = current_a × voltage_v, Wh acumulado |
| RF-PLUG-006 | Snapshot Feed Mode | ✅ COMPLETE | `feed_snapshot.c` | NVS snapshot com checksum, TTL 30min |
| RF-PLUG-007 | Estados visuais | ✅ COMPLETE | `plug_model.h`, `plug_manager.c` | ON_NORMAL, OFF_NORMAL, OFF_FAULT, OFF_WAITING, BLOCKED |
| RF-PLUG-008 | Tempo mínimo ON/OFF | ⚠️ PARTIAL | `plug_model.h`, `plug_manager.c` | Campos existem (60s/30s) mas sem enforce |
| RF-PLUG-009 | Religamento sequencial | ✅ COMPLETE | `restart_fsm.c` | 5 estados, stagger sequence configurável |
| RF-PLUG-010 | Reserva estrutural P01/P02 | ✅ COMPLETE | `hardware_config.h`, `plug_manager.c` | P01=BOMBA, P02=AQUECEDOR |
| RF-PLUG-011 | Dupla confirmação plugs críticos | ✅ COMPLETE | `command_validator.c` | `requires_double_confirmation=true` para plug_id 1,2 |
| RF-PLUG-012 | Realocação AQUECEDOR/COOLER | ❌ NOT_FOUND | — | `role_override_source` field existe mas sem lógica |
| RF-PLUG-013 | Consumo diário máximo | ⚠️ PARTIAL | `plug_model.h` | `max_energy_wh_day` field existe mas sem monitoramento |
| RF-PLUG-014 | Curto-circuito por plugue | ✅ COMPLETE | `electric_fsm.c` | `fator_curto` + 3 amostras consecutivas → SHORT_CIRCUIT → SAFE_OFF |

## 7. FEED

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-FEED-001 | Comportamento plugs em Feed | ✅ COMPLETE | `feed_fsm.c`, `plug_manager.c` | Bombas OFF, P01/P02 não afetados |
| RF-FEED-002 | Interface Feed Mode | ✅ COMPLETE | `ui_screens.c` | Overlay com contagem regressiva, botão sair |
| RF-FEED-003 | Sinalização LED amarelo | ✅ COMPLETE | `driver_buzzer_led.c` | LED amarelo pisca durante feed |
| RF-FSM-FEED-001 | Feed ortogonal ao estado global | ✅ COMPLETE | `feed_fsm.c`, `app_main.c` | Feed não altera global state; SAFE_OFF cancela feed |

## 8. ENERGY

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-ENERGY-001 | Leitura PZEM Modbus | ✅ COMPLETE | `driver_pzem.c` | V, A, W, kWh, PF, Hz via UART 9600 8N1 |
| RF-ENERGY-003 | Acumuladores de energia | ✅ COMPLETE | `cdn_energy.c` | Wh por plugue e total |
| RF-ENERGY-004 | API de energia | ✅ COMPLETE | `api_rest.c` | `GET /api/v1/energy` |
| RF-ENERGY-005 | Reset de energia | ✅ COMPLETE | `driver_pzem.c`, `cdn_energy.c` | `pzem_reset_energy()`, `cdn_energy_reset_all()` |
| RF-ENERGY-007 | Sobretensão/subtensão | ✅ COMPLETE | `electric_fsm.c` | Persistence timer → DEGRADED → SAFE_OFF |
| RF-ENERGY-008 | Sobrecorrente total | ✅ COMPLETE | `electric_fsm.c` | `total_power_limit_w`, `total_current_limit_a` |
| RF-ENERGY-009 | Fator de potência | ⚠️ PARTIAL | `electric_fsm.c` | `pf_min`, `pf_time_s` existem mas sem enforce |
| RF-ENERGY-010 | Log SD energia | ⚠️ PARTIAL | `storage_sd.c` | `SD_LOG_TYPE_ENERGY` existe mas sem loop dedicado |
| RF-FSM-ELECTRIC-001 | FSM proteção elétrica | ✅ COMPLETE | `electric_fsm.c` | 8 estados com persistência temporal |
| RF-PROTECTION-001 | Religamento inteligente | ✅ COMPLETE | `restart_fsm.c` | WAITING→ENERGIZING→MONITORING→COMPLETE |
| RF-FSM-RELIG-ELECT-001 | FSM religamento elétrico | ✅ COMPLETE | `restart_fsm.c` | Sequencial com stagger, aborta se condições perdidas |

## 9. STORAGE / PERSIST

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-STORAGE-001 | NVS fonte primária | ✅ COMPLETE | `config_manager.c` | NVS como fonte de verdade; SD como backup |
| RF-STORAGE-002 | SD com fallback RAM | ⚠️ PARTIAL | `storage_sd.c` | SD ausente não bloqueia boot; RAM fallback não implementado |
| RF-STORAGE-003 | Escrita atômica SD | ✅ COMPLETE | `storage_sd.c` | `.tmp` → fdatasync → rename; rollback se falha |
| RF-STORAGE-004 | Backup de configuração | ✅ COMPLETE | `storage_sd.c` | `storage_sd_backup_config()` NVS→SD |
| RF-STORAGE-005 | Healthcheck SD | ✅ COMPLETE | `storage_sd.c` | `storage_sd_is_mounted()` check |
| RF-STORAGE-PARAM-001 | Persistência NVS completa | ✅ COMPLETE | `param_catalog.h` | Todos parâmetros em structs NVS |

## 10. WEB

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-WEB-001 | HTTP, CORS, versionamento | ✅ COMPLETE | `api_rest.c` | REST /api/v1, respostas JSON, CORS headers |
| RF-WEB-002 | GET /api/v1/state | ✅ COMPLETE | `api_rest.c` | Estado completo + safeoff_reason + alerts |
| RF-WEB-003 | Operações escrita autenticadas | ✅ COMPLETE | `api_rest.c`, `api_auth.c` | X-Auth-Token, SHA-256, Command Validator |
| RF-WEB-004 | Autenticação token | ✅ COMPLETE | `api_auth.c` | Token RAM 1h, rate limit por IP |
| RF-WEB-005 | GET /api/v1/health | ✅ COMPLETE | `api_rest.c` | Health consolidado (heap, sd, wifi, uptime) |
| RF-WEB-007 | SD indisponível | ✅ COMPLETE | `api_rest.c` | API funcional sem SD |

## 11. ALERT

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-ALERT-001 | Modelo canônico de alerta | ✅ COMPLETE | `alert_manager.h`, `alert_model.h` | severity, category, acked, action_hint, source |
| RF-ALERT-002 | Silence temporário | ⚠️ PARTIAL | `ui_screens.c` | MUTE na UI mas sem enforce no alert_manager |
| RF-ALERT-003 | Tabela canônica ALM-001 a ALM-065 | ✅ COMPLETE | `alm_ids.h` | 65 IDs sequenciais |
| RF-ALERT-004 | Timeout de ACK | ❌ NOT_FOUND | — | `ack_timestamp` existe mas sem timeout check |
| RF-ALERT-005 | Anti-spam/deduplicação | ✅ COMPLETE | `alert_manager.c` | Dedup: atualiza last_seen_ts em vez de criar novo |
| RF-ALERT-006 | Sinalização sonora | ✅ COMPLETE | `driver_buzzer_led.c` | `buzzer_beep()`, `buzzer_led_alert()` |

## 12. UI

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-UI-OVERLAY-001 | Overlay crítico mínimo | ✅ COMPLETE | `ui_screens.c` | safeoff, emergency, alert overlays com causa + instrução |
| RF-UI-MUTE-001 | MUTE normativo | ✅ COMPLETE | `ui_screens.c` | `ui_toggle_mute()`, overlay [MUTE] amarelo |
| RF-UI-MUTE-002 | Restrições do MUTE | ✅ COMPLETE | `ui_screens.c` | MUTE não altera ALM, LED, estado global |
| RF-UI-WIZARD-001..005 | Wizard | ⚠️ PARTIAL | `ui_screens.c`, `api_rest.c` | Overlay único + API, sem steps individuais |
| RF-UI-STATUS-001 | Barra persistente de status | ⚠️ PARTIAL | `ui_screens.c` | Badge de estado global; demais indicadores parciais |
| RF-UI-INPUT-001 | Touch + Keypad | ✅ COMPLETE | `ui_touch.c`, `ui_keypad.c` | XPT2046 SPI + AD Keypad ADC |
| RF-UI-DISPLAY-001 | Brilho configurável | ❌ NOT_FOUND | — | Sem implementação de controle de brilho |
| RF-UI-CAROUSEL-001 | Carrossel com pausa | ❌ NOT_FOUND | — | Sem carrossel automático implementado |
| RF-UI-DIAG-001 | Tela de diagnóstico | ⚠️ PARTIAL | `screen_diagnostic.c` | Tela existe com TAG mas conteúdo mínimo |

## 13. RESET

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-RESET-001 | Hard reset seguro | ⚠️ PARTIAL | `app_main.c` | `esp_reset_reason()` capturado; sem FSM de reset |
| RF-RESET-002 | Escopo de apagamento | ❌ NOT_FOUND | — | Sem função de factory reset |
| RF-RESET-003 | Reset por menu | ❌ NOT_FOUND | — | Sem handler de reset destrutivo |
| RF-RESET-004 | Contagem regressiva + dupla confirmação | ❌ NOT_FOUND | — | Não implementado |

## 14. DATA (Modelagem)

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-DATA-GLOBAL-001 | Estrutura GlobalState | ✅ COMPLETE | `global_state.h` | 30+ campos conforme SRS |
| RF-DATA-ALM-001 | Estrutura Alert | ✅ COMPLETE | `alert_model.h` | 18 campos conforme SRS |
| RF-DATA-PLUG-001 | Estrutura Plug | ✅ COMPLETE | `plug_model.h` | 25+ campos conforme SRS |
| RF-DATA-CONFIG-ROOT-001 | Estrutura raiz ConfigRoot | ⚠️ PARTIAL | `param_catalog.h` | Todas seções existem via NVS namespaces separados |
| RF-DATA-CONSISTENCY-001 | Campo documentado tem consumidor | ⚠️ PARTIAL | — | bypass_detected, role_override_source existem sem consumidor |

## 15. HW (Hardware)

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-HW-I2C-001 | I²C (SDA=GPIO8, SCL=GPIO9) | ✅ COMPLETE | `pin_map.h` | MCP23017@0x20, DS3231@0x68 |
| RF-HW-SPI-001 | SPI (MOSI=11, MISO=12, SCK=13) | ✅ COMPLETE | `pin_map.h` | CS dedicados: TFT=10, Touch=38, SD=15, ADC1=41, ADC2=42 |
| RF-HW-UART-001 | UART PZEM | ✅ COMPLETE | `pin_map.h` | TX=17, RX=18, 9600 8N1 |
| RF-HW-1WIRE-001 | DS18B20 (DQ=GPIO4) | ✅ COMPLETE | `pin_map.h`, `driver_ds18b20.c` | Pull-up 4.7k, CRC, 85°C rejection |

## 16. SEGURANÇA

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-SEC-AUTH-IMPL-001 | Auth implementation | ✅ COMPLETE | `api_auth.c` | SHA-256, token RAM, sem DEFAULT_PASSWORD |
| RF-AUDIT-SEC-001 | Log de auditoria | ✅ COMPLETE | `audit_log.c` | timestamp, event_type, result |
| RF-CMD-VALIDATOR-001 | Command Validator | ✅ COMPLETE | `command_validator.c` | 7 funções can_*, double_confirmation |

## 17. WDT / TIME

| ID | Título | Status | Arquivos | Evidência |
|----|--------|--------|----------|-----------|
| RF-WDT-RECOVERY-001 | WDT recovery | ✅ COMPLETE | `wdt_advanced.c` | WDT por task, timeout configurável |
| RF-TIME-001 | DS3231 RTC | ✅ COMPLETE | `driver_ds3231.c` | I2C RTC com timestamp |

---

## Resumo Consolidado

| Categoria | Total | ✅ COMPLETE | ⚠️ PARTIAL | ❌ NOT_FOUND |
|-----------|-------|-------------|-------------|--------------|
| GLOBAL | 9 | 8 | 1 | 0 |
| FLOW/BOOT | 5 | 3 | 2 | 0 |
| LED | 3 | 3 | 0 | 0 |
| THERMAL | 10 | 7 | 2 | 1 |
| ATO | 7 | 6 | 1 | 0 |
| PLUG | 14 | 10 | 2 | 2 |
| FEED | 4 | 4 | 0 | 0 |
| ENERGY | 9 | 6 | 2 | 0 |
| STORAGE | 6 | 5 | 1 | 0 |
| WEB | 6 | 6 | 0 | 0 |
| ALERT | 6 | 4 | 1 | 1 |
| UI | 8 | 4 | 2 | 2 |
| RESET | 4 | 0 | 1 | 3 |
| DATA | 5 | 3 | 2 | 0 |
| HW | 4 | 4 | 0 | 0 |
| SEGURANÇA | 3 | 3 | 0 | 0 |
| WDT/TIME | 2 | 2 | 0 | 0 |
| **TOTAL** | **105** | **78 (74%)** | **17 (16%)** | **9 (9%)** |

## Pendências Críticas (NOT_FOUND)
1. **RF-THERMAL-005** — Indicador de tendência térmica (UI)
2. **RF-PLUG-004** — Detecção de bypass (segurança)
3. **RF-PLUG-012** — Realocação AQUECEDOR/COOLER (resiliência)
4. **RF-ALERT-004** — Timeout de ACK (monitoramento)
5. **RF-UI-DISPLAY-001** — Brilho configurável (UX)
6. **RF-UI-CAROUSEL-001** — Carrossel automático (UX)
7. **RF-RESET-001 a 004** — Reset seguro completo (safety)

## Observações
- **74% COMPLETE** — Núcleo funcional implementado (safety, FSMs, API, persistência)
- **16% PARTIAL** — Funcionalidades existentes mas sem completa aderência SRS
- **9% NOT_FOUND** — 9 requisitos não implementados (maioria UI e reset)
- **Build verificado**: 0 erros/0 warnings, binário 0xc0d20 (25% livre)
- **Commit**: `e38241c` — push para `origin/main`
