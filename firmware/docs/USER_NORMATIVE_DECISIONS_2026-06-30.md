# Decisoes Normativas do Usuario — 2026-06-30

Documento de rastreabilidade das definicoes fornecidas pelo usuario para fechar NCs de compliance.

## 1. Backup diario (TXT no SD)

| Parametro | Valor |
|-----------|-------|
| Horario | 01:00 (RTC local) |
| Formato | TXT |
| Local | `/sdcard/config/backup/` |
| Nomenclatura | `DDINO_BKUP_{NN}_{DDMMYY}.txt` |
| Retencao | 5 arquivos (rotacao no 6o) |
| Conteudo | Thresholds (thermal/ATO/electric/feed/restart), todos os plugues, ultimos 10 logs de eventos, rede (SSID/IP/RSSI **sem senha WiFi**) |

Implementacao: `services/storage_sd_schedule.c` → `storage_sd_tick_schedules()`.

## 2. Delay P01/P02 na inicializacao

| Parametro | Valor |
|-----------|-------|
| Plugues | P01 (Aquecedor), P02 (Cooler) |
| Delay | 5000 ms apos boot |
| Constante | `HW_RELAY_P01_P02_STARTUP_DELAY_MS` |

Implementacao: `services/plug_manager.c` → `plug_startup_delay_active()`.

## 3. Log horario (SD)

| Parametro | Valor |
|-----------|-------|
| Frequencia | 1 arquivo por hora (minuto 00) |
| Local | `/sdcard/logs/hourly/` |
| Nomenclatura | `LOG{HH}de{DD}{MM}{AA}.txt` (ex.: `LOG14de300626.txt`) |
| Campos | `data_hora;estados_plugs;amperes_plugs;pzem_w;pzem_v;status_sensores` |

Implementacao: `services/storage_sd_schedule.c` → `write_hourly_log_txt()`.

## 4. Wizard

Manter **6 telas** (sem expansao ate fase de prototipagem).

## 5. Tecla MUTE

| Parametro | Valor |
|-----------|-------|
| Gesto | Segurar tecla **UP** do keypad por 5 s |
| UI | Icone `LV_SYMBOL_MUTE` vermelho na topbar, ao lado do WiFi (mesmo tamanho) |
| Feedback | Hint inline "MUTE ativo" por 3 s |

Implementacao: `drivers/driver_ad_keypad_lvgl.c`, `ui_topbar.c`, `ui_app.c`.

## 6. Tecla HOME

| Parametro | Valor |
|-----------|-------|
| Gesto | Duplo clique na tecla **ENTER** (central) em ≤ 500 ms |
| Acao | Dashboard imediato |

Implementacao: `driver_ad_keypad_gesture_poll()` → `LV_KEY_HOME`.

## 7. Nomes de plugues P03–P10

| Plug | Nome |
|------|------|
| P01 | Aquecedor (fixo) |
| P02 | Cooler (fixo) |
| P03–P10 | Preset da biblioteca ou nome customizado |

Presets (`services/plug_preset_catalog.c`):

- Bomba Recalque — `LV_SYMBOL_REFRESH`
- Circulacao — `LV_SYMBOL_LOOP`
- Bomba Reposicao ATO — `LV_SYMBOL_DOWNLOAD`
- Iluminacao — `LV_SYMBOL_IMAGE`
- Skimmer — `LV_SYMBOL_SHUFFLE`
- Aquecedor / Cooler (referencia para P03–P10)

API: `plug_manager_apply_preset()`, `plug_manager_set_custom_name()`.

## 8. Sensor de pH

| Parametro | Valor |
|-----------|-------|
| Modulo | Analogico (AliExpress ref. 1005004359126943) |
| GPIO | **GPIO 7** (`PIN_PH_ADC_GPIO`) |
| ADC | ADC1 CH6 |
| Health | `SUB_SENSOR_PH` em `app_main.c` |

Implementacao: `drivers/driver_ph_sensor.c`, `pin_map.h`.
