<!-- @requirement RF-DOC-001, RF-DOC-002, RF-DOC-CLASS-001, RB-DOC-001, RB-DOC-002 -->
# Classificação de Documentos

| Tipo | Sigla | Exemplos                        |
|------|-------|---------------------------------|
| UM   | Manual do Usuário               | user_manual.md                 |
| SRS  | Software Requirements Spec      | SRS_*.txt                     |
| RDD  | Requirements-Driven Design      | DECISIONS.md                  |
| STD  | Standard / Norma                | STATE_RULES.md                |
| HW   | Hardware Spec                   | BOM.md, pin_map.h             |
| SW   | Software Spec                   | interfaces dos módulos .h     |

- RF-DOC-001: Documentação publicada em components/docs/.
- RF-DOC-002: README.md na raiz com instruções de build.
- RB-DOC-001: Toda função pública tem Doxygen.
- RB-DOC-002: Toda struct pública tem descrição dos campos.
