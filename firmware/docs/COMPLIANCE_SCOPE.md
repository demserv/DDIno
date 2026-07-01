# Escopo de Compliance — SRS v3.11 (Política do Projeto)

> **Data:** 2026-07-01 · **Baseline:** SRS v3.11 + Adendo pH + Errata §49.1  
> **Meta de software:** **≥ 95%** de conformidade **código + wiring funcional** antes de qualquer montagem de hardware.

---

## 1. O que entra no cálculo de ≥ 95%

| Categoria | Incluído | Critério |
|-----------|----------|----------|
| Implementação | `main/`, `core/`, `drivers/`, `services/`, `fsm/`, `web/`, `ui/`, `include/` | RF implementado e **cabeado** em runtime (não stub/código morto) |
| Ciclo de vida ALM | `alm_monitor.c`, `alert_manager.c`, donos únicos | raise + clear/auto-clear conforme catálogo §49 |
| Config / persistência | `config_manager.c`, `config_export.c`, NVS/SD | Parâmetros operacionais via ConfigRoot; export/import com CRC |
| API / contrato | `api_rest.c` | Rotas, campos e erros alinhados ao §23 |
| UI / HMI funcional | `ui/hmi/*` | Telas registradas, eventos tratados, overlays e menus §73 |
| Segurança lógica | `safety_controller.c`, `command_validator.c`, `plug_manager.c` | SAFE_OFF/EMERGENCY, bloqueio de comandos, proteções elétricas em software |
| Rastreabilidade | Tags `@requirement`, RTM, docs de auditoria | Evidência arquivo:função por RF crítico |

**Peso sugerido para nota global (sem testes/hardware):**

| Domínio | Peso |
|---------|------|
| Segurança / SAFE_OFF / ACK | 30% |
| FSM global + boot + restart | 25% |
| Alertas / ALM | 15% |
| API / Web / persistência | 15% |
| UI / HMI / UX funcional | 15% |

---

## 2. O que está **fora** do cálculo de ≥ 95%

Decisão explícita do solicitante: **não penalizar** o score de compliance por itens abaixo.

| Categoria | Motivo | Quando entra |
|-----------|--------|--------------|
| **Montagem / bancada / flash / smoke test** | Hardware só será montado após software ≥ 95% SRS | Pós sign-off de software |
| **Testes Unity / `idf.py test` / CI de testes** | Escopo de QA separado; não bloqueia meta de software | Opcional, após ≥ 95% |
| **Diretório `test/`** | Suítes unitárias — úteis, mas **não** contam no placar | Manutenção contínua |
| **E2E em placa real** | Validação física (relés, sensores, SD, touch) | Após montagem |
| **Build reproduzível em CI limpo** | Evidência desejável, mas **não** reduz score de RF | Paralelo ao fechamento de gaps |
| **Frameworks de terceiros** | `managed_components/`, Unity, LVGL vendor | Apenas referência de versão |

---

## 3. Sequência de gate (ordem obrigatória)

```
1. Fechar gaps P0–P4 de software (auditoria + RTM)
2. Reauditoria honesta → cada domínio ≥ 9,5/10 (≥ 95% ponderado)
3. Sign-off de software (este documento + AUDITORIA_COMPLIANCE_2026-07-01.md)
4. Só então: montagem de hardware + flash + smoke/E2E físico
5. Opcional: ampliar cobertura com Unity/CI (não retroage o score de software)
```

---

## 4. Artefatos oficiais de compliance de software

| Artefato | Papel |
|----------|-------|
| `docs/COMPLIANCE_SCOPE.md` | **Este documento** — política de escopo |
| `docs/AUDITORIA_COMPLIANCE_2026-07-01.md` | Placar honesto por domínio + plano P0–P4 |
| `docs/RTM_DELTA_COMPLIANCE_2026-07-01.md` | Delta implementado e deferidos |
| `docs/PLANO_ACAO_COMPLIANCE_95.md` | Plano de ação (Fases A–B = software; C–D = pós-95%) |

Relatórios legados (`COMPLIANCE_REPORT.md`, `COMPLIANCE_FINAL_REPORT.md`, seção 10 de
`AUDITORIA_COMPLIANCE_RIGOR_2026-06-30.md`) **não** substituem a reauditoria 2026-07-01
e podem superestimar itens não cabeados.

---

## 5. Próximo passo pós-reauditoria

Reauditoria concluída: **~69%** software (`REAUDITORIA_COMPLIANCE_2026-07-01.md`).
Executar Fases R1→R3 em `PLANO_ACAO_COMPLIANCE_95.md` para fechar ~26 pp até ≥95%.
