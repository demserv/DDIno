<!-- @requirement RB-STOR-001..017, RF-PERSIST-001..015 -->
# Regras de Armazenamento

- RB-STOR-001: NVS namespace por domínio (cfg, cal, profile, state, log, wiz, time, alm).
- RB-STOR-002: Schema versionado. Chave "schema_version" obrigatória em cada NS.
- RB-STOR-003: default_value seguro em NOT_FOUND.
- RB-STOR-004: nvs_commit() após cada batch.
- RB-STOR-005: Migração automática em schema_version_persistido < schema_version_atual.
- RB-STOR-006: Blobs com CRC32 para integridade.
- RB-STOR-007: SD como backup do NVS.
- RB-STOR-008: Operações atômicas (set → commit) para dados críticos.
- RB-STOR-009: Rollback via restore de snapshot.
- RB-STOR-010: Wear leveling via nvs_flash (gerenciado pelo driver ESP-IDF).
- RB-STOR-011: Falha persistente do SD → system_state = DEGRADED.
- RB-STOR-012: Timeout de operação SD: 5s.
- RF-PERSIST-001: Persistência de configurações em NVS.
- RF-PERSIST-ATOMIC-001: Atomicidade set+commit.
- RF-PERSIST-EXPORT-001: Export JSON para SD.
- RF-PERSIST-POWERLOSS-001: Snapshot automático pré-powerloss.
