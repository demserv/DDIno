# Relatório Final — Sprint Compliance 95%

Branch: `sprint/compliance-95-percent`

Data: 2026-06-27

Build: **SKIPPED** (ESP-IDF não instalado neste PC — conforme instrução do operador)

## Tarefas T-01..T-18

| Tarefa | Status |
|--------|--------|
| T-01 | OK — task_ui_fn/task_web_fn estáticas removidas |
| T-02 | OK — drivers renomeados; ui/ legado ausente |
| T-03 | OK — ui_lvgl_tick + mutex |
| T-04 | OK — GET /api/v1/state |
| T-05 | OK — system_state_to_str(prev) |
| T-06 | OK — audit_log RAM fallback |
| T-07 | OK — stubs data/web/bom removidos |
| T-08 | OK — alert_manager unificado |
| T-09 | OK — storage_facade |
| T-10 | OK — conf_ctl removido |
| T-11 | OK — thermal via FSM |
| T-12 | OK — electric via config_get_electric |
| T-13 | OK — anotações @requirement + tools/rtm_gen.py |
| T-14 | OK — docs/HARDCODES_RESIDUAIS.md |
| T-15 | OK — InlineHint + overlay Z-order |
| T-16 | OK — WiFi backoff + PWM LEDC |
| T-17 | OK — testes Unity adicionados |
| T-18 | OK — este relatório |

## Verificações finais

Stubs eliminados: data_ctl, web_ctl, bom_ctl, alm_ctl, conf_ctl, alert_manager_ext  
UI legada ui/: ausente  
lv_timer_handler: protegido via ui_lvgl_tick  
/api/v1/state: registrado

## Próximo passo

Instalar ESP-IDF e executar `idf.py reconfigure && idf.py build`.
