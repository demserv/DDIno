# API_ERROR_CODES

| Fonte | `include/system_types.h` — `canonical_error_code_t` |
| API | `web/api_rest.c` — respostas JSON `{ "error": "...", "code": N }` |

| Código | Nome | HTTP | Descrição | Quando |
|--------|------|------|-----------|--------|
| 0 | ERR_AUTH_REQUIRED | 401 | Autenticação necessária | Token ausente |
| 1 | ERR_AUTH_INVALID | 401 | Credencial inválida | Login falhou |
| 2 | ERR_AUTH_FORBIDDEN | 403 | Sem permissão | Role insuficiente |
| 3 | ERR_STATE_NOT_ALLOWED | 409 | Estado não permite ação | SAFE_OFF/EMERGENCY |
| 4 | ERR_PLUG_BLOCKED | 409 | Plugue bloqueado | blocked=true |
| 5 | ERR_VALIDATION_ERROR | 400 | Parâmetro inválido | command_validator |
| 6 | ERR_CONFIG_INVALID | 400 | Configuração inválida | NVS/schema |
| 7 | ERR_SCHEMA_INCOMPATIBLE | 400 | Schema incompatível | versão config |
| 8 | ERR_STORAGE_UNAVAILABLE | 503 | SD/NVS indisponível | persistência |
| 9 | ERR_HARDWARE_UNAVAILABLE | 503 | Hardware indisponível | sensor/driver fail |
| 10 | ERR_SAFE_MODE_ACTIVE | 409 | Modo seguro ativo | SAFE_OFF |
| 11 | ERR_MONITOR_ONLY_ACTIVE | 409 | Somente monitoramento | monitor_only |
| 12 | ERR_RATE_LIMITED | 429 | Rate limit | api_rate_limit |
| 13 | ERR_INTERNAL_ERROR | 500 | Erro interno | fallback |

Todos os POST de carga passam por `command_validator_validate()` antes de atuar no hardware.
