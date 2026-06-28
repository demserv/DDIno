# AC_SAFETY_CHECKLIST

| Requisito | RNF-SAFETY-AC / AF.3 |

## Checklist normativo

| # | Item | Evidência firmware | Status |
|---|------|-------------------|--------|
| 1 | Protoboard AC proibida | `HW_AC_PROTOBOARD_FORBIDDEN=1` | COMPLIANT |
| 2 | Dupont AC proibido | `HW_AC_DUPONT_FORBIDDEN=1` | COMPLIANT |
| 3 | Gabinete plástico isolante | `HW_AC_PLASTIC_ENCLOSURE=1` | COMPLIANT |
| 4 | Fusível entrada principal | `HW_AC_MAIN_INPUT_FUSE=1` | COMPLIANT |
| 5 | Fusível por tomada | `HW_AC_FUSE_PER_OUTLET=1` | COMPLIANT |
| 6 | Módulo relé 5V optoacoplado JD-VCC | `HW_RELAY_MODULE_HAS_OPTO`, `HAS_JD_VCC` | COMPLIANT |
| 7 | Buck 3.3 V e 5 V dedicados | `HW_BUCK_3V3_REQUIRED`, `HW_BUCK_5V_REQUIRED` | COMPLIANT |
| 8 | Corrente máx 10 A/tomada | `HW_AC_MAX_CURRENT_PER_OUTLET_A=10` | COMPLIANT |
| 9 | Bivolt suportado | `HW_MAINS_BIVOLT_SUPPORTED=1` | COMPLIANT |
| 10 | Boot relés OFF | `relay_init_safe()` em boot | COMPLIANT |
| 11 | SAFE_OFF desliga todas cargas | `relay_all_off()` | COMPLIANT |
| 12 | Sobrecorrente → SAFE_OFF | `electric_fsm.c` | COMPLIANT |
| 13 | Curto por plug → BLOCKED | `plug_manager.c` | COMPLIANT |
| 14 | Heater+cooler simultâneo → OFF | `thermal_fsm.c` | COMPLIANT |

Fonte: `include/hardware_config.h`, `core/safety_controller.c`, `services/electric_fsm.c`
