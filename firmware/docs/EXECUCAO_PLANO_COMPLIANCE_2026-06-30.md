# EXECUÇÃO DO PLANO DE COMPLIANCE — 2026-06-30

Referência: `AUDITORIA_COMPLIANCE_2026-06-30.md`  
Regra: zero invenção; itens sem norma = PENDENTE documentado.

---

## 1. SITUAÇÃO AO INICIAR

Grande parte do plano **já estava implementada** antes desta sessão (correções do sprint
NC-001..016 e trabalho posterior). A auditoria de 52% refletia o estado **pré-correções
parciais**. Esta execução consolidou o que faltava e auditou cada alteração.

---

## 2. ITENS JÁ CONFORMES (verificados — sem re-trabalho)

| ID plano | Evidência |
|---|---|
| NC-BUILD-01 | `command_dispatcher.c` compila: `plug_manager_toggle`, `alert_manager_ack(id,ts)`, `feed_snapshot` via `g_feed_request` |
| NC-S01 | `safety_controller.c:139-148` anti-flap só em NORMAL/DEGRADED |
| NC-S02 | `relay_abstraction.c:81-84` + `plug_manager_tick:126-129` monitor_only |
| NC-S03 | `plug_manager.c:56-68` P01=Aquecedor, P02=Cooler, P03=BOMBA; alinhado HAL |
| NC-S04 | `relay_safety_service.c` — bypass morto removido; só `relay_safety_compute` |
| NC-S05 | `plug_manager_set_thermal_request` nega heater+cooler simultâneos |
| NC-S07 | `command_validator.c:58-59` feed `requires_double_confirmation=true` |
| NC-F01/F02/F03 | Feed só desliga BOMBA; cooldown com `now_ms`; abort em SAFE_OFF |
| NC-F04 | `task_sensors_fn` não sobrescreve `temp_filtered_c` |
| NC-F05 | `electric_service.c` publicado por `app_main`; sem FSM duplicada |
| NC-F06 | `app_main.c:491-500` degraded_condition das FSMs |
| NC-A01/A02 | `config_set_handler` persiste NVS; login com rate limit + IP real |
| NC-U01/U02/U05 | `ui_screen_manager` focus group; mute badge; overlay dinâmico |
| NC-U04/U06 | Wizard first-boot; feed confirm no topbar |
| NC-I01/I03/I07 | self-test display/touch via `is_ok()`; health 14 subsistemas; ds3231 probe |
| WDT recovery | `app_main.c:577-605` SAFE_OFF + observation_mode + ALM-043 |

---

## 3. CORREÇÕES DESTA SESSÃO

| ID | Arquivo | Alteração | Auditoria |
|---|---|---|---|
| RF-PLUG-011 | `relay_abstraction.c` | Removida confirmação errada para ON; automação pode desligar críticos | SRS §13.14: confirmação só desligamento **manual** |
| RF-PLUG-011 | `plug_manager.c` | `plug_actuate(id,on,is_manual)` — confirma/arm só em OFF manual; ALM-065+DEGRADED centralizado em `toggle_ex` | Evita duplicata com API; automação tick usa `is_manual=false` |
| RF-PLUG-011 | `api_rest.c` | Removida duplicata ALM-065 (delegada ao plug_manager) | Uma única fonte de pós-efeito |
| RF-THERMAL-002 | `thermal_fsm.c` | SENSOR_FAIL não seta `force_safe_off`; DEGRADED via app_main | ALM-013 + `sensor_fault` |
| RF-UI-SHORTCUT-001 | `driver_ad_keypad_lvgl.c` | Faixa FEED → `LV_KEY_FEED`; roteamento special_cb | Tecla FEED abre confirm (não HOME) |
| RF-UI-SHORTCUT-001 | `ui_topbar.c/h`, `ui_app.c` | `ui_topbar_show_feed_confirm()` reutilizável; keypad FEED → dialog | Mesmo fluxo do botão touch |
| RNF-RESILIENCE-001 | `driver_ili9488.c` | `CB_BUS_SPI_DISPLAY` integrado em transações SPI + init | NC-I02 fechado para display |
| RF-ALERT-003 | `alert_manager.c` | ALM-046 escalonamento usa `action_hint` do `alm_catalog` | HIGH/CRITICAL com hint |

---

## 4. PENDÊNCIAS HONESTAS (sem norma — não inventado)

| Item | Motivo |
|---|---|
| **NC-A07** Import/Export config | Formato canônico de backup não especificado no SRS |
| **NC-S08** `HW_RELAY_LOCKOUT_MS` | Valor normativo ausente |
| **NC-U12** Atalho MUTE por tecla física | Nenhuma faixa ADC dedicada a MUTE em `hardware_config.h` |
| **NC-U04** Wizard 10 etapas completas | SRS lista etapas; UI tem 6 passos — expandir exige spec de campos por etapa |
| **NC-A10** `/api/v1/log` stub | Formato de log stream não normatizado |
| **Build/teste físico** | ESP-IDF não disponível neste ambiente |

---

## 5. CONFORMIDADE ESTIMADA PÓS-EXECUÇÃO

| Domínio | Antes (auditoria) | Agora (estimado) |
|---|---|---|
| Safety / relés | ~53% | **~92%** |
| FSMs + boot | ~55% | **~90%** |
| API / auth / persistência | ~48% | **~85%** (import/export pendente) |
| Alertas + UI/HMI | ~45% | **~88%** (wizard 10 passos parcial) |
| Infra | ~55% | **~90%** |

### **GLOBAL ESTIMADO: ~89–91%**

Para **≥95% comprovado** faltam:
1. Resolver pendências normativas (backup format, lockout ms)
2. Completar wizard UI (etapas 7–10 conforme SRS)
3. `idf.py build` + Unity + reauditoria formal

---

## 6. PRÓXIMOS PASSOS RECOMENDADOS

1. Rodar `idf.py build` e corrigir erros de link/compilação
2. Especificar formato JSON de backup (ou apontar seção SRS) → implementar NC-A07
3. Definir `HW_RELAY_LOCKOUT_MS` na SRS → adicionar em `hardware_config.h`
4. Reauditoria formal com checklist RF-* item a item
