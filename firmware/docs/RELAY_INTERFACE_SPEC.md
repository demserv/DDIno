# RELAY_INTERFACE_SPEC — Especificacao da Interface de Rele

## Visao Geral
Define a interface eletrica e logica entre o modulo ESP32-S3 e o modulo de reles externo (8 canais, 5V, optoacoplado).

## Pinos de Controle (via MCP23017)
| Rele | GPIO MCP | Logica | Notas |
|------|----------|--------|-------|
| P03  | MCP_GP0  | Active HIGH | Tomada 1 |
| P04  | MCP_GP1  | Active HIGH | Tomada 2 |
| P05  | MCP_GP2  | Active HIGH | Tomada 3 |
| P06  | MCP_GP3  | Active HIGH | Tomada 4 |
| P07  | MCP_GP4  | Active HIGH | Tomada 5 |
| P08  | MCP_GP5  | Active HIGH | Tomada 6 |
| P09  | MCP_GP6  | Active HIGH | Tomada 7 |
| P10  | MCP_GP7  | Active HIGH | Tomada 8 |

### Drivers onboard (ESP32-S3 direto)
| Rele | GPIO | Logica | Notas |
|------|------|--------|-------|
| P01  | GPIO5 | Active HIGH | Saida de uso geral |
| P02  | GPIO6 | Active HIGH | Saida de uso geral |

## Caracteristicas Eletricas
- **Tensao do modulo**: 5V (via buck dedicado `HW_BUCK_5V_REQUIRED`)
- **Corrente maxima por rele**: 10A a 250VAC (item 8 da AC_SAFETY_CHECKLIST)
- **Isolamento**: Optoacoplador com jumper JD-VCC (remove VCC do lado logico)
- **Estado de boot**: Reles em OFF garantido por `relay_init_safe()`
- **Pull-down**: Resistores de pull-down de 10k Ohm em cada saida do MCP23017

## Logica de Controle
- `RELAY_P01_ACTIVE_LEVEL = 1` (active HIGH)
- `RELAY_P02_ACTIVE_LEVEL = 1` (active HIGH)
- `relay_on(plug_id)` → escreve HIGH no bit correspondente
- `relay_off(plug_id)` → escreve LOW no bit correspondente
- `relay_all_off()` → escreve 0x00 em todos os registradores do MCP23017 + GPIO5/6 LOW

## Seguranca
1. Boot com reles OFF (`relay_init_safe()`)
2. Transicao para SAFE_OFF dispara `relay_all_off()` (RF-GLOBAL-004)
3. Curto-circuito em tomada → BLOCKED state em plug_manager + rele OFF
4. Heater + Cooler simultaneo proibido → ambos OFF (thermal_fsm)
5. Watchdog de hardware (RWDT) reinicia ESP32-S3 se task_safety_core travar

## Referencias
- `drivers/driver_relay.c` — Driver I2C para MCP23017
- `drivers/relay_abstraction.c` — Abstracao relay_on/off/all_off
- `include/hardware_config.h` — Definicoes de pinos e niveis
- `docs/AC_SAFETY_CHECKLIST.md` — Checklist de seguranca AC
