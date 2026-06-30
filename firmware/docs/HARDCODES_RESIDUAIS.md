# Relatório de Hardcodes Residuais

Gerado em: 2026-06-27

## services/

```
services/thermal_service.c:12: s_setpoint = 25.0f
services/thermal_service.c:16: temp_normal_c=25.0f, temp_critical_c=32.0f, temp_extreme_c=38.0f
services/plug_manager.c:64: current_limit_a = 10.0f (default plug)
services/plug_manager.c:74: fator_curto = 3.0f
services/self_test.c:143: limites sanity DS18B20
```

## fsm/

```
fsm/thermal_fsm.c:72-73: conversão dt_ms → minutos
```

## core/

Nenhum literal float operacional residual em `core/*.c`.

## Notas pós T-12

Hardcodes elétricos `1500`/`10.0f` removidos de `electric_service.c`.
