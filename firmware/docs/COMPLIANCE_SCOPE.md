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

**Peso para nota global (somente software):**

| Domínio | Peso |
|---------|------|
| Segurança / SAFE_OFF / ACK | 30% |
| FSM global + boot + restart | 25% |
| Alertas / ALM | 15% |
| API / Web / persistência | 15% |
| UI / HMI / UX funcional | 15% |

**Método de auditoria:** reauditoria **domínio a domínio**, evidência **função a função** (L3: código ligado + caller de produção).

**Dual gate de sign-off:**
- Cada um dos **14 domínios** ≥ **9,5/10**
- Cada um dos **5 buckets** ponderados ≥ **95%**

---

## 2. O que **nunca** entra no cálculo de ≥ 95%

Decisão explícita do Owner: estes itens **não aumentam nem reduzem** o placar de compliance de software.

| Categoria | Tratamento |
|-----------|------------|
| **Smoke manual / flash / bancada / E2E físico** | **Fora do placar.** Só após sign-off software ≥95%. |
| **Testes Unity / `idf.py test` / CI de testes / `test/`** | **Fora do placar.** QA opcional pós-sign-off. |
| **Build reproduzível em CI** | Evidência paralela; **não** altera score. |
| **Frameworks de terceiros** | Referência de versão apenas (`managed_components/`, LVGL, Unity vendor). |

Auditorias **não** podem:
- Penalizar por ausência de smoke ou testes automatizados
- Exigir smoke ou testes como pré-requisito para calcular a nota de software
- Tratar “falta de smoke” como gap de RF

---

## 3. Sequência de gate (ordem obrigatória)

```
1. Fechar gaps de software (auditoria domínio a domínio + RTM)
2. Reauditoria L3 → 14 domínios ≥ 9,5/10 E buckets ≥ 95%
3. Sign-off de software (COMPLIANCE_SCOPE + REAUDITORIA_*_FINAL.md)
4. Montagem de hardware
5. Flash + smoke manual + E2E físico (validação operacional — fora do placar)
6. Opcional: Unity/CI (fora do placar)
```

---

## 4. Artefatos oficiais

| Artefato | Papel |
|----------|-------|
| `docs/COMPLIANCE_SCOPE.md` | **Este documento** — política de escopo |
| `docs/REAUDITORIA_COMPLIANCE_2026-07-01-FINAL.md` | Placar pós H1–H3 (domínio a domínio) |
| `docs/AUDITORIA_COMPLIANCE_2026-07-01.md` | Baseline histórico + plano P0–P4 |
| `docs/RTM_DELTA_COMPLIANCE_2026-07-01.md` | Delta implementado |

Relatórios legados (`COMPLIANCE_REPORT.md`, seção 10 revogada de `AUDITORIA_COMPLIANCE_RIGOR_2026-06-30.md`) **não** substituem a reauditoria FINAL.

---

## 5. Estado atual (pós H1–H3)

| Métrica | Valor |
|---------|:-----:|
| Global ponderado | **~90%** |
| Domínios ≥ 9,5 | **0 / 14** |
| Veredito software | **REPROVADO** (gap ~5 pp) |

Próximo passo **somente software:** fechar P1 em `REAUDITORIA_COMPLIANCE_2026-07-01-FINAL.md` §5 e reauditar domínios afetados.
