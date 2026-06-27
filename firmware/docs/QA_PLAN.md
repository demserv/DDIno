<!-- @requirement RB-QA-001..015, RNF-QA-001, TC-QA-TRACE-001 -->
# QA Plan — Monitor Aquário Inteligente

## Static Analysis
- clang-tidy: TODOS os módulos
- cppcheck: todos os .c/.h
- Verificação automática de @requirement tags

## Unit Tests (Unity Framework)
- storage_manager tests
- relay_abstraction tests (invariants)
- event_bus tests (pub/sub)
- health_matrix tests

## Hardware Tests
- Self-test boot: crítico
- Ciclo de relés com lockout
- Leitura de sensores com valores conhecidos

## Integration Tests
- app_main boot sequence completo
- SAFE_OFF → NORMAL recovery
- Wizard fluxo completo
