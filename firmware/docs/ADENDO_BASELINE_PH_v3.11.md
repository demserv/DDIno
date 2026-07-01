# Adendo de Baseline — Sensor de pH (SRS v3.11)

| Campo | Valor |
|-------|-------|
| Documento | Adendo de Baseline — Módulo de pH |
| Natureza | Errata controlada pós-Bloco 13/N (SRS §1.2, item 1 — prevalência máxima) |
| Data | 2026-07-01 |
| Status | NORMATIVO para desenvolvimento, integração e QA |
| Decisão do Owner | Manter o sensor de pH no produto e incorporá-lo à baseline |

---

## 1. Justificativa e prevalência

A SRS Técnica Consolidada v3.11 (§1.3) listava sensores químicos (incluindo pH) como
"FORA DA BASELINE FINAL". Por **decisão registrada do Owner (2026-07-01)**, o módulo de pH
passa a ser **componente normativo** do produto.

Este adendo é registrado como **errata controlada** conforme a regra de prevalência da
SRS §1.2 (item 1 — Errata de Normalização tem prioridade máxima) e o mecanismo de
erratas controladas descrito em §1.1 (item 7).

Este adendo **NÃO cria novos IDs de ALM** (proibido por SRS §1.5; tabela ALM-001..065
está FECHADA por §1.6). O pH reutiliza o ALM canônico existente para calibração/sensor.

---

## 2. Escopo do módulo de pH

- Sensor de pH analógico (módulo comercial), leitura via ADC (`driver_ph_sensor.c`).
- Faixa física do sensor: pH 0–14 (`HW_PH_SENSOR_SPAN`, constante de hardware).
- Função: **telemetria/monitoramento** da qualidade da água.
- **Não é safety-critical**: advertências de pH **não** forçam SAFE_OFF nem EMERGENCY.

---

## 3. Requisitos normativos RF-PH-*

### RF-PH-001 — Leitura de pH
| Campo | Valor |
|-------|-------|
| ID | RF-PH-001 |
| Prioridade | MÉDIA |
| Módulo responsável | drivers/driver_ph_sensor.c, services/sensor_ctl |
| Descrição | O sistema deve ler o pH via ADC e disponibilizá-lo à UI/API como telemetria. |
| Evidência | `driver_ph_sensor.c: ph_sensor_read()` |

### RF-PH-002 — Faixas de advertência configuráveis
| Campo | Valor |
|-------|-------|
| ID | RF-PH-002 |
| Prioridade | MÉDIA |
| Módulo responsável | services/config_manager, services/alm_monitor |
| Descrição | Faixas de advertência de pH devem ser **parâmetros configuráveis** (não hardcode — RF-GLOBAL-005), persistidos em ConfigRoot/NVS. |
| Parâmetros (defaults normativos) | `warn_low_ph=6.5`, `warn_high_ph=8.5` |
| Evidência | `param_catalog.h: ph_params_storage_t`; `config_manager.c: config_get_ph/config_set_ph` |

### RF-PH-003 — Calibração e ALM canônico
| Campo | Valor |
|-------|-------|
| ID | RF-PH-003 |
| Prioridade | MÉDIA |
| Módulo responsável | services/alm_monitor |
| Descrição | Leitura de pH fora da faixa de calibração `[calib_min_ph, calib_max_ph]` deve gerar **ALM-049** (Falha de calibração/inconsistência de sensor — SRS §49). Nenhum ID novo é criado. |
| Parâmetros (defaults normativos) | `calib_min_ph=4.0`, `calib_max_ph=10.0` |
| ALM relacionado | **ALM-049** (canônico) |

### RF-PH-004 — pH não é safety-critical
| Campo | Valor |
|-------|-------|
| ID | RF-PH-004 |
| Prioridade | CRÍTICA (restrição de segurança) |
| Descrição | Advertência de pH fora de faixa é **telemetria/log**; **não** deve forçar SAFE_OFF nem EMERGENCY, e **não** deve reutilizar os IDs canônicos ALM-038/039 (que pertencem ao ATO — SRS §49). |
| Restrição | ALM-038 = "Frequência anormal de refill" (ATO); ALM-039 = "Reservatório ATO vazio/bloqueado". Estes voltam ao domínio ATO. |

---

## 4. Correção de conformidade associada (referência à SRS §49)

Antes deste adendo, o firmware reutilizava indevidamente os IDs canônicos de ATO para pH:

| ID | SRS §49 (canônico) | Uso indevido anterior | Correção |
|----|--------------------|-----------------------|----------|
| ALM-038 | Frequência anormal de refill (ATO) | pH baixo | Devolvido ao ATO (Fase 1) |
| ALM-039 | Reservatório ATO vazio/bloqueado | pH alto | Devolvido ao ATO (Fase 1) |
| ALM-049 | Falha calibração/inconsistência sensor | parcial | pH fora de calibração → ALM-049 |

---

## 5. BOM

| Item | Classificação |
|------|---------------|
| Módulo sensor de pH analógico | Incluído por necessidade técnica de integração — validar em revisão de hardware (SRS §1.5, exceção permitida) |

---

## 6. Rastreabilidade

| RF | Código | Persistência |
|----|--------|--------------|
| RF-PH-001 | `driver_ph_sensor.c` | — |
| RF-PH-002 | `param_catalog.h`, `config_manager.c`, `config_export.c` | ConfigRoot + NVS `cfg_ph` |
| RF-PH-003 | `alm_monitor.c` (ALM-049) | — |
| RF-PH-004 | `alm_monitor.c` (sem SAFE_OFF), `ato_fsm.c` (ALM-038/039 devolvidos) | — |
