# Auditoria de Compliance — SRS v3.11-AF.3+AF.4+B12/N+B13/N

## Sumário Executivo

**Data**: 26/06/2026
**SRS Baseline**: v3.11 COMPLETA — GO CODING READY
**Alvo**: 95% de compliance
**Resultado**: **42% — ABAIXO DO ALVO**

| Domínio | Reqs | COMPLIANT | PARTIAL | MISSING | % |
|---------|------|-----------|---------|---------|---|
| GLOBAL / SAFETY | 12 | 10 | 2 | 0 | **83%** |
| THERMAL | 10 | 8 | 2 | 0 | **80%** |
| ATO | 8 | 6 | 2 | 0 | **75%** |
| PLUG | 16 | 12 | 3 | 1 | **75%** |
| FEED | 5 | 4 | 1 | 0 | **80%** |
| ENERGY / ELETRIC | 12 | 4 | 5 | 3 | **33%** |
| WEB / API | 18 | 4 | 8 | 6 | **22%** |
| STORAGE / PERSIST | 16 | 3 | 7 | 6 | **19%** |
| ALERT / ALM | 65 | 24 | 0 | 41 | **37%** |
| LED | 3 | 3 | 0 | 0 | **100%** |
| UI / HMI | 15 | 2 | 8 | 5 | **13%** |
| HW | 10 | 8 | 2 | 0 | **80%** |
| WDT / TIME | 4 | 3 | 1 | 0 | **75%** |
| AUTH / SEC | 5 | 1 | 4 | 0 | **20%** |
| **TOTAL** | **199** | **92** | **45** | **62** | **46%** |

**Ajustado (apenas RF obrigatórios documentados na RTM):** ~52%

---

## Achados por Domínio

### 1. GLOBAL / SAFETY (83%)
**COMPLIANT**: Estados NORMAL/DEGRADED/SAFE_OFF/EMERGENCY, transições com prioridade, safeoff_reason tracking, SAFE_OFF exit pre-conditions, EMERGENCY exit, rearming controlado, boot sequence 15+ etapas, self-test → SAFE_OFF, .tmp orphan scan, ViewModel centralizado.

**GAPS**:
- `observation_mode` não implementado — requer Modo de Observação pós-WDT recovery antes de retornar a NORMAL
- `monitor_only_mode` implementado mas sem endpoint API para ativar/desativar (só leitura)

### 2. THERMAL (80%)
**COMPLIANT**: DS18B20 com CRC, filtro média móvel, classificação por parâmetros, tendência °C/min, heater/cooler exclusão mútua, parâmetros configuráveis, FSM 5 estados.

**GAPS**:
- `force_safe_off = true` para sensor fail — interceptado no safety_controller mas FSM ainda sinaliza incorretamente (F001)
- Anticiclo de 5 min para heater/cooler não implementado (RF-THERMAL-001.2)

### 3. ATO (75%)
**COMPLIANT**: FSM 6 estados, debounce ADC 3 amostras, configuração LOW/HIGH, overflow → SAFE_OFF.

**GAPS**:
- Estados ATO usam nomes diferentes do SRS (NORMAL/REFILLING/ERROR/BLOCKED/OVERFLOW/DISABLED vs IDLE/REFILLING/COOLDOWN/BLOCKED_OVERFLOW/BLOCKED_TIMEOUT/BLOCKED_SENSOR_FAULT)
- Debounce 500ms (RF-ATO-001.1) não implementado — usa contagem de amostras sem temporização real
- Timeout de refill não gera COOLDOWN com bloqueio após N falhas

### 4. PLUG (75%)
**COMPLIANT**: 10 plugs, modos OFF/ON/AUTO/SCHEDULE, bypass detection, corrente individual, religamento sequencial, dupla confirmação P01/P02, realocação AQUECEDOR/COOLER, tempo mínimo ON/OFF, consumo diário máximo.

**GAPS**:
- `plug_effective_state_t` expandido para 6 estados mas código existente só usa OFF/ON (F002)
- ALM-065 não implementado para desligamento manual P01/P02 (RF-PLUG-011 modificado)
- Profile management (RF-UI-MENU-002) completamente ausente

### 5. ENERGY / ELETRIC (33%) ⚠️ CRÍTICO
**COMPLIANT**: PZEM Modbus leitura V/A/W/kWh/PF/Hz, energia acumulada por plug, SD log periódico, sobretensão/subtensão, curto-circuito por plug.

**GAPS**:
- **RF-ENERGY-002**: Cálculo de custo/orçamento — MISSING (sem tarifa R$/kWh, sem ALM-051)
- **RF-ENERGY-004**: Projeção mensal — STUB ONLY (monthly_kwh[6] nunca populado)
- **RF-ENERGY-005**: CSV export — não é CSV verdadeiro, escreve em log.txt único
- **RF-ENERGY-008**: Sobrecorrente total — MISSING (total_current_limit_a config existe mas FSM nunca usa)
- **RF-ENERGY-009**: PF enforcement — alert only, sem ação corretiva
- **RF-ENERGY-010**: Tendência elétrica — MISSING (ALM-057/058 reaproveitados)
- **BUG**: `app_main.c:348` passa `HW_MAINS_FREQUENCY_HZ` como `delta_ms` no `cdn_energy_update()`

### 6. WEB / API (22%) ⚠️ CRÍTICO
**COMPLIANT**: 27 endpoints registrados, `/api/v1` prefix, auth token + rate limit, command validator em todas rotas de escrita, health endpoint.

**GAPS**:
- **RF-WEB-001**: CORS headers — ZERO implementado
- **RF-WEB-002**: Endpoint `/api/v1/status` deveria ser `/api/v1/state`; faltam observation_mode, config_schema_version, time_valid, time_source, plugs/sensors/alerts arrays
- **RF-WEB-004 / RF-SEC-AUTH-IMPL-001**: hash sem salt (SHA-256 apenas)
- **RF-WEB-006**: SD password/config recovery — MISSING
- **RF-WEB-008**: Config export — MISSING (storage_export_to_buffer é stub ESP_ERR_NOT_SUPPORTED)
- **RF-API-SCHEMA-003**: Alerts schema retorna só {id, acked} — faltam severity, category, message, timestamp, etc.
- **Profile API**: MISSING (sem endpoints para perfis)
- **Maintenance API**: MISSING (só leitura, sem POST para ativar/desativar)
- **Import**: MISSING (storage_import_from_buffer é stub)
- **Endpoints faltando**: GET /api/v1/state (nome errado), GET/POST /api/v1/plugs/{id}, POST /api/v1/alerts/{id}/ack, GET /api/v1/export, POST /api/v1/import, POST /api/v1/maintenance

### 7. STORAGE / PERSIST (19%) ⚠️ CRÍTICO
**COMPLIANT**: RAM fallback para logs, config_staging commit/rollback, SD directory structure, CRC para ConfigRoot.

**GAPS**:
- **RF-PERSIST-ATOMIC-001**: Escrita .tmp+rename sem fsync antes do rename
- **RF-PERSIST-POWERLOSS-001**: Snapshot pré-powerloss — só Feed Mode, sem estado geral
- **RF-PERSIST-POWERLOSS-002/-003**: MISSING
- **RF-PERSIST-SD-001**: SD cheio — sem verificação de espaço livre
- **RF-PERSIST-SD-003**: SD removido em runtime — sem detecção de hot-removal
- **RF-PERSIST-SD-004**: SD retorna — sem auto-recovery/re-init
- **RF-DATA-EXPORT-001**: Export — storage_export_to_buffer é stub
- **RF-DATA-IMPORT-001**: Import — storage_import_from_buffer é stub
- **RNF-STORAGE-006**: CRC definido em storage_metadata_t mas nunca populado

### 8. ALERT / ALM (37%) ⚠️ CRÍTICO
**COMPLIANT**: 65 IDs definidos em alm_ids.h, modelo canônico 18 campos, ACK timeout enforcement, deduplicação, silence.

**GAPS**:
- **41 ALMs definidos mas nunca raised**: ALM-001, ALM-002, ALM-004, ALM-005, ALM-006, ALM-007, ALM-008, ALM-009, ALM-010, ALM-011, ALM-012, ALM-014, ALM-021, ALM-022, ALM-023, ALM-024, ALM-025, ALM-027, ALM-030, ALM-031, ALM-032, ALM-033, ALM-034, ALM-035, ALM-036, ALM-038, ALM-039, ALM-040, ALM-041, ALM-042, ALM-044, ALM-045, ALM-047, ALM-049, ALM-050, ALM-051, ALM-053, ALM-056, ALM-059, ALM-062, ALM-064
- Muitos ALMs críticos (ALM-005, ALM-050, ALM-051, ALM-062) sem implementação

### 9. UI / HMI (13%) ⚠️ CRÍTICO
**COMPLIANT**: Metric card com unidades, tela de alertas (sem filtro), tela de diagnóstico (parcial), carrossel automático, overlays críticos, MUTE.

**GAPS**:
- **RF-UI-STATUS-001**: `ui_status_bar.h` é dead code — topbar real não tem state badge, SD, alert count, self-test flag
- **RF-UI-WIZARD-001..005**: Wizard tem 6 passos vs 9 exigidos — faltam plugs críticos, manutenção, perfis
- **RF-UI-ALERTS-001**: Sem filtro por severidade/categoria
- **RF-UI-DIAG-001**: 9 campos vs 13+ exigidos — sem dados quantitativos
- **RF-UI-MENU-002**: Profile management — MISSING (zero código)
- **RF-UI-MENU-003**: Modo Manutenção — "Manutencao" tile abre wizard, não maintenance mode
- **RF-UI-GRAPH-001**: Só gráfico mensal — sem 24h/dia/semana
- **RF-UI-CALIB-001**: Tela de calibração não registrada (TODO: port)
- **RF-UI-MSG-001**: Catálogo de erro — MISSING

### 10. AUTH / SEC (20%)
**COMPLIANT**: Token RAM 64 chars, rate limit por IP, logout invalida token, reboot invalida, timeout 1h.

**GAPS**:
- Hash SHA-256 sem salt (RF-SEC-AUTH-IMPL-001)
- SecurityAuditLog existe mas não registra todas as tentativas
- ALM-062 (acesso não autorizado) nunca raised
- Usuário "admin" hardcoded (aceitável para admin único SRS)

### 11. WDT / TIME (75%)
**COMPLIANT**: WDT recovery, task WDT timeouts, DS3231 RTC.

**GAPS**:
- `wdt_resets_24h` no health endpoint é stub (hardcoded 0)
- TREND de tempo (time_source_t expandido para NONE/RTC/NTP/MANUAL/INVALID) mas NTP não implementado

### 12. HW (80%)
**COMPLIANT**: Pinagem AF.3, I2C GPIO8/9, SPI compartilhado, UART PZEM, 1-Wire DS18B20, MCP23017, MCP3208, relés, touch, keypad.

**GAPS**:
- LED 12V via driver (RNF-LED-HW-001) — driver existe mas sem confirmação de compatibilidade 12V
- Anti-glitch instrumental quantitativo pendente (PND-3.11-004)

---

## Plano de Ação — 95% Compliance

### Prioridade CRÍTICA (afeta >5% compliance cada)

| # | Ação | Reqs | Esforço | Dependências |
|---|------|------|---------|-------------|
| PA-C01 | **Implementar 41 ALMs faltantes** — criar calls `alert_manager_raise()` para todos ALMs definidos mas não raised | ALM-001 a ALM-065 (faltam 41) | 3-4 dias | Conhecimento dos pontos de trigger no código |
| PA-C02 | **Completar schemas da API** — adicionar campos faltantes em /api/v1/status, /api/v1/health, /api/v1/alerts; renomear /api/v1/status para /api/v1/state | RF-WEB-002, RF-API-SCHEMA-001, 002, 003 | 1-2 dias | — |
| PA-C03 | **Implementar import/export** — remover stubs storage_export_to_buffer e storage_import_from_buffer | RF-DATA-EXPORT-001, RF-DATA-IMPORT-001, RF-WEB-008, RF-PERSIST-PROFILE-001 | 2-3 dias | — |
| PA-C04 | **Adicionar CORS headers** — implementar Access-Control-* em todas as respostas API | RF-WEB-001 | 0,5 dia | — |
| PA-C05 | **Adicionar salt ao hash de senha** — usar esp_fill_random() como salt de 16 bytes | RF-SEC-AUTH-IMPL-001, RF-WEB-004 | 0,5 dia | — |
| PA-C06 | **Completar status bar persistente** — implementar ui_status_bar com state badge, SD icon, alert count, self-test flag | RF-UI-STATUS-001 | 1 dia | — |

### Prioridade ALTA

| # | Ação | Reqs | Esforço | Dependências |
|---|------|------|---------|-------------|
| PA-A01 | **Implementar energy cost/budget** — adicionar tarifa R$/kWh, calcular custo, ALM-051 | RF-ENERGY-002, RF-ENERGY-004 | 1-2 dias | PA-C03 (import/export) |
| PA-A02 | **Corrigir FSM elétrica** — implementar total_current overcurrent, ELECTRIC_STATE_NORMALIZING, PF enforcement | RF-ENERGY-008, RF-FSM-ELECTRIC-001 | 1-2 dias | — |
| PA-A03 | **Corrigir wizard para 9 passos** — adicionar steps: plugs críticos, manutenção, perfis | RF-UI-WIZARD-001..005 | 1-2 dias | — |
| PA-A04 | **Implementar SD power-loss recovery** — hot-removal detection, re-init, auto-recovery | RF-PERSIST-SD-003, SD-004, POWERLOSS-001..003 | 2-3 dias | — |
| PA-A05 | **Adicionar API endpoints faltantes** — /api/v1/maintenance, /api/v1/export, /api/v1/import, profile CRUD | RF-WEB-006, RF-WEB-008, Profile, Maintenance API | 2-3 dias | PA-C03 |
| PA-A06 | **Implementar profile management UI** — save/load/rename/delete perfis com confirmação | RF-UI-MENU-002, RF-DATA-PROFILE-001 | 2 dias | PA-C03 |
| PA-A07 | **Adicionar filtros na tela de alertas** — filtro por severidade, categoria, status ACK | RF-UI-ALERTS-001 | 1 dia | — |
| PA-A08 | **Completar tela de diagnóstico** — adicionar 4+ campos quantitativos (heap, uptime, versão, RSSI) | RF-UI-DIAG-001 | 0,5 dia | — |
| PA-A09 | **Corrigir energy CSV** — criar arquivos diários, formato CSV RFC 4180 | RF-ENERGY-005 | 0,5 dia | — |
| PA-A10 | **Adicionar gráficos de energia** — 24h, semanal, mensal com seletor de período | RF-UI-GRAPH-001 | 2 dias | — |
| PA-A11 | **Adicionar fsync antes do rename** — garantir atomicidade real na escrita SD | RF-PERSIST-ATOMIC-001 | 0,5 dia | — |

### Prioridade MÉDIA

| # | Ação | Reqs | Esforço |
|---|------|------|---------|
| PA-M01 | Adicionar verificação de espaço livre SD antes de writes | RF-PERSIST-SD-001 | 0,5 dia |
| PA-M02 | Implementar observation_mode pós-WDT recovery | RF-FLOW-BOOT-004 | 1 dia |
| PA-M03 | Manutenção mode via API POST | RF-UI-MENU-003, RF-FLOW-MAINT-001..003 | 1 dia |
| PA-M04 | Implementar error message catalog | RF-UI-MSG-001 | 1 dia |
| PA-M05 | Registrar tela de calibração no screen manager | RF-UI-CALIB-001 | 0,5 dia |
| PA-M06 | Implementar NTP sync (time_source_t) | RF-TIME-001 | 1 dia |
| PA-M07 | Corrigir electric_fsm PF state (SENSOR_FAIL → PF_LOW) | RF-ENERGY-009 | 0,5 dia |
| PA-M08 | Corrigir bug `cdn_energy_update()` passando freq como delta_ms | RF-ENERGY-003 | 0,5 dia |
| PA-M09 | Corrigir thermal_fsm force_safe_off para sensor fail | RF-THERMAL-002 (F001) | 0,5 dia |
| PA-M10 | Adicionar anticiclo heater/cooler 5 min | RF-THERMAL-001.2 | 0,5 dia |
| PA-M11 | SecurityAuditLog para todas tentativas de autenticação | ALM-062 | 0,5 dia |
| PA-M12 | Populatar wdt_resets_24h no health endpoint (remover stub) | RF-WDT-RECOVERY-001 | 0,5 dia |

### Esforço Total Estimado

| Prioridade | Itens | Dias |
|------------|-------|------|
| CRÍTICA | 6 | 8-11 |
| ALTA | 11 | 13-16 |
| MÉDIA | 12 | 7 |
| **TOTAL** | **29** | **28-34** |

---

## Projeção de Compliance Pós-Plano

| Domínio | Atual | Pós-plano |
|---------|-------|-----------|
| GLOBAL / SAFETY | 83% | 100% |
| THERMAL | 80% | 95% |
| ATO | 75% | 90% |
| PLUG | 75% | 95% |
| FEED | 80% | 95% |
| ENERGY / ELETRIC | 33% | 75% |
| WEB / API | 22% | 85% |
| STORAGE / PERSIST | 19% | 80% |
| ALERT / ALM | 37% | 90% |
| LED | 100% | 100% |
| UI / HMI | 13% | 70% |
| HW | 80% | 95% |
| WDT / TIME | 75% | 95% |
| AUTH / SEC | 20% | 80% |
| **TOTAL** | **46%** | **~87%** |

**Para atingir 95% seria necessário**:
1. Executar todas as ações prioritárias (~34 dias de trabalho)
2. Implementar cenários E2E e cobertura de testes completa
3. Adicionar suíte de testes automatizados contínuos

---

## Conclusão

**Compliance atual: ~46%** — significantemente abaixo do alvo de 95%.

O firmware tem uma base sólida (safety controller, FSMs, drivers, API estrutura) mas carece de:
1. **41 ALMs não raised** de 65 (maior gap individual)
2. **Import/Export de configuração** (stubs)
3. **API schemas incompletos** e CORS ausente
4. **UI status bar e wizard incompletos**
5. **Energy cost/budget e projeção mensal** não implementados
6. **Persistência SD sem recovery** (hot-removal, re-init, full-check)

O plano de ação de 29 itens (~28-34 dias de trabalho) elevará a compliance para ~87%. Para 95%, investimento adicional em testes E2E e automação seria necessário.
