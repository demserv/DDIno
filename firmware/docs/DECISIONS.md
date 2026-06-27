<!-- @requirement D-3.11-001..006, PND-3.11-001..009 -->
# Decisões Arquiteturais (ADRs)

## D-3.11-001 — Comunicação via event_bus pub/sub
**Contexto:** Módulos precisam trocar informações sem acoplamento direto.
**Decisão:** event_bus com queue FreeRTOS interna + subscribers por callback. Eventos serializados.

## D-3.11-002 — Relay abstraction com lockout
**Contexto:** Relés precisam de proteção anti-chatter e invariantes heater×cooler.
**Decisão:** relay_abstraction com lockout temporal e verificação de exclusividade.

## D-3.11-003 — NVS schema versionado
**Contexto:** Configurações precisam migrar quando o schema evolui.
**Decisão:** storage_manager com schema_version persistido por namespace + função migrate automaticamente no init.

## D-3.11-004 — UI via ViewModel + eventos
**Contexto:** UI LVGL não deve acessar hardware diretamente.
**Decisão:** ui_view_model provê dados mock; ações de usuário viram eventos via event_bus.

## D-3.11-005 — Multi-task FreeRTOS
**Contexto:** Sistema precisa de prioridades separadas para safety, sensores, UI, rede.
**Decisão:** 9 tasks com prioridades fixas e pinagem a core dedicado.

## D-3.11-006 — Partições OTA dupla
**Contexto:** SRS exige OTA com rollback.
**Decisão:** factory + ota_0 + ota_1 em 16MB flash.
