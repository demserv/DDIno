# FreeRTOS Task Map — Monitor Aquário Inteligente

## Normative Reference
SRS Técnico Consolidado Final v3.11-AF.3+AF.4+B12/N+B13/N §3.5
Documento obrigatório: firmware/docs/FREERTOS_TASK_MAP.md

## Task Table

| # | Task ID | Nome | Prioridade | Stack | Core | Período | WDT | Heartbeat |
|---|---------|------|-----------|------|------|---------|-----|-----------|
| 0 | TASK_ID_SAFETY_CORE | `safety_core` | 10 (Muito alta) | 4096 | 1 | 50ms | 2000ms | Sim |
| 1 | TASK_ID_SENSORS | `sensors` | 9 (Alta) | 4096 | 1 | 200ms | 3000ms | Sim |
| 2 | TASK_ID_PLUG_CONTROL | `plug_control` | 8 (Alta) | 4096 | 1 | 100ms | 2000ms | Sim |
| 3 | TASK_ID_STORAGE | `storage` | 6 (Média) | 4096 | 1 | 1000ms | 5000ms | Sim |
| 4 | TASK_ID_UI | `ui` | 5 (Média) | 8192 | 0 | 10ms | 5000ms | Sim |
| 5 | TASK_ID_WEB | `web` | 4 (Baixa/Média) | 8192 | 0 | 50ms | 10000ms | Sim |
| 6 | TASK_ID_DIAG | `diag` | 2 (Baixa) | 3072 | 0 | 1000ms | 10000ms | Opcional |

## Core Assignment

- **Core 1 (APP_CPU)**: safety_core, sensors, plug_control, storage
- **Core 0 (PRO_CPU)**: ui, web, diag

Rationale: Safety-critical and sensor tasks isolated on APP_CPU.
UI and web tasks on PRO_CPU to prevent UI/network from blocking safety.

## Rules

1. Falha da task Web não pode travar safety.
2. Falha da task UI não pode travar safety.
3. Storage não pode bloquear fail-safe.
4. Safety core deve ter heartbeat monitorado.
5. WDT deve distinguir falha crítica e não crítica.

## Task Descriptions

### safety_core (TASK_ID 0)
- **Responsabilidade**: GlobalState, safety controller, avaliação de falhas, transições de estado global (NORMAL/DEGRADED/SAFE_OFF/EMERGENCY)
- **Heartbeat**: Obrigatório. Falha → WDT panic → reset
- **Comunicação**: Recebe eventos via event_bus, escreve global_state
- **WDT crítico**: Timeout 2000ms

### sensors (TASK_ID 1)
- **Responsabilidade**: Leitura DS18B20 (temperatura), MCP3208 ADC (ATO, keypad), PZEM (energia), ACS712 (corrente por plug)
- **Heartbeat**: Obrigatório
- **Comunicação**: Publica leituras no event_bus para safety_core e FSMs

### plug_control (TASK_ID 2)
- **Responsabilidade**: Plug Manager, relés P01-P10, controle de estado dos plugs
- **Heartbeat**: Obrigatório
- **Comunicação**: Recebe comandos via event_bus/command_validator, atualiza estado dos relés via relay_abstraction

### storage (TASK_ID 3)
- **Responsabilidade**: NVS (fonte primária), SD card (logs/backup), energy logging, feed snapshot
- **Heartbeat**: Obrigatório
- **Comunicação**: Publica status SD no event_bus

### ui (TASK_ID 4)
- **Responsabilidade**: LVGL rendering, touch (XPT2046), keypad, telas, carrossel, overlays
- **Heartbeat**: Obrigatório
- **Stack maior** (8192): LVGL requer heap para buffers de framebuffer e objetos

### web (TASK_ID 5)
- **Responsabilidade**: Servidor HTTP REST API /api/v1, autenticação, rate limiting
- **Heartbeat**: Obrigatório
- **Stack maior** (8192): httpd_handle requer tasks internas

### diag (TASK_ID 6)
- **Responsabilidade**: Health matrix, circuit breaker updates, diagnóstico, logs periódicos
- **Heartbeat**: Opcional (não crítico)
- **Período**: 1000ms (execução lenta tolerada)

## WDT Strategy

| Falta | Impacto | Ação WDT |
|-------|---------|----------|
| safety_core sem heartbeat | Perda de proteção fail-safe | WDT panic → reset (2000ms) |
| sensors sem heartbeat | Leituras obsoletas | WDT reset (3000ms) |
| plug_control sem heartbeat | Relés sem atualização | WDT panic → reset (2000ms) |
| storage sem heartbeat | Logs atrasados | WDT reset (5000ms) |
| ui sem heartbeat | Tela congelada | Não crítico (5000ms) |
| web sem heartbeat | API fora do ar | Não crítico (10000ms) |
| diag sem heartbeat | Diagnóstico atrasado | Ignorado (opcional) |

## Source Files

- `include/task_manager.h` — Definições de tasks, prioridades, stacks, cores
- `services/task_manager.c` — Criação e gerenciamento de tasks
- `services/wdt_advanced.c` — Watchdog por task com auto-registro
- `main/app_main.c` — Inicialização e lançamento de tasks

## Related Requirements

- RNF-FREERTOS-001: Mapa de tasks FreeRTOS
- ALM-043: Task/módulo sem heartbeat
- TC-FREERTOS-001..003: Testes de tasks FreeRTOS
