# PARAMETER_CATALOG

| Versão | 1.0.0 (`PARAM_CATALOG_VERSION`) |
| Fonte código | `include/param_catalog.h`, `services/config_manager.c` |
| Persistência | NVS namespaces `cfg_*` |

| Parâmetro | Módulo | Tipo | Faixa | Default | Wizard | NVS | Safety | TC |
|-----------|--------|------|-------|---------|--------|-----|--------|-----|
| temp_normal_c | thermal | float °C | SRS faixa aquário | 25.0 | Sim step THERMAL | cfg_thermal | Sim | TC-CFG-TH-001 |
| temp_critical_c | thermal | float °C | > normal | 32.0 | Sim | cfg_thermal | Sim SAFE_OFF | TC-CFG-TH-002 |
| temp_extreme_c | thermal | float °C | > critical | 38.0 | Sim | cfg_thermal | Sim EMERGENCY | TC-CFG-TH-003 |
| hysteresis_c | thermal | float °C | >0 | 1.0 | Sim | cfg_thermal | Sim | TC-CFG-TH-004 |
| low_level_adc | ATO | int32 | 0–4095 | catalog | Sim step ATO | cfg_ato | Sim | TC-CFG-ATO-001 |
| high_level_adc | ATO | int32 | > LOW | catalog | Sim | cfg_ato | Sim | TC-CFG-ATO-002 |
| overflow_margin_adc | ATO | int32 | >0 | catalog | Sim | cfg_ato | Sim OVERFLOW | TC-CFG-ATO-003 |
| refill_timeout_s | ATO | uint32 s | >0 | catalog | Sim | cfg_ato | Sim | TC-CFG-ATO-004 |
| total_power_limit_w | electric | float W | >0 | 1500 | Sim step ELECTRIC | cfg_electric | Sim | TC-CFG-EL-001 |
| per_plug_current_limit_a | electric | float A | >0 | 10.0 | Sim | cfg_electric | Sim | TC-CFG-EL-002 |
| pf_min | electric | float | 0–1 | catalog | Sim | cfg_electric | Sim | TC-CFG-EL-003 |
| acs offset/gain | ACS712 | float | calibrável | por canal NVS | Wizard calibração | cfg_acs | Degradado se inválido | TC-CFG-ACS-001 |
| tempo_min_estabilizacao_s | antiflap | uint32 | >0 | 10 | Não | cfg_aflap | Sim transições | TC-CFG-AFL-001 |
| janela_flap_s | antiflap | uint32 | >0 | 60 | Não | cfg_aflap | Sim | TC-CFG-AFL-002 |
| max_transicoes_flap | antiflap | uint32 | >0 | 3 | Não | cfg_aflap | Sim | TC-CFG-AFL-003 |
| cooldown_reentrada_s | antiflap | uint32 | >0 | 300 | Não | cfg_aflap | Sim | TC-CFG-AFL-004 |
| feed_duration_min | feed | uint32 min | >0 | catalog | API/UI | cfg_feed | Não viola safety | TC-CFG-FEED-001 |
| monitor_only_mode | system | bool | — | false | API | cfg_sys | Bloqueia relés | TC-CFG-MON-001 |
| wizard_completed | system | bool | — | false | Wizard | cfg_sys | Bloqueia auto | TC-CFG-WIZ-001 |
| WDT timeout | watchdog | uint32 ms | HW bounds | 2000 | Não | HW const | Sim | TC-WDT-001 |

**Regra**: Parâmetros operacionais não hardcoded — defaults em `param_catalog.h`, runtime via `config_manager`.
