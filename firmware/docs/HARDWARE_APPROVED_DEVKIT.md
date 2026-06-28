# HARDWARE_APPROVED_DEVKIT

| Campo | Valor normativo (SRS AF.3) |
|-------|------------------------------|
| MCU | ESP32-S3 |
| DevKit baseline | ESP32-S3-DevKitC-1 N16R8 |
| Flash | 16 MB |
| PSRAM | 8 MB (OPI) |
| Framework | ESP-IDF v5.2.2 |
| Display | TFT 3.5" ILI9488 480×320 SPI |
| Touch | XPT2046 SPI CS GPIO38 |
| RTC | DS3231 I2C 0x68 |
| Expansor | MCP23017 I2C 0x20 (relés P03–P10, LEDs) |
| ADC | MCP3208 #1/#2 SPI CS GPIO41/42 |
| Energia | PZEM-004T v4.0 UART GPIO17/18 |
| Temp | DS18B20 1-wire GPIO4 |
| SD | microSD SPI CS GPIO15 |

## Pinagem SPI (RF-HW-SPI-MUTEX-001)

| Sinal | GPIO |
|-------|------|
| MOSI | 11 |
| MISO | 12 |
| SCK | 13 |
| TFT CS | 10 |
| SD CS | 15 |
| Touch CS | 38 |
| MCP3208 #1 CS | 41 |
| MCP3208 #2 CS | 42 |

## Pinagem I2C (RF-HW-I2C-001)

| Sinal | GPIO |
|-------|------|
| SDA | 8 |
| SCL | 9 |

## Relés (RF-HW-RELAY-LOGIC-001)

| Plug | Interface |
|------|-----------|
| P01 | GPIO5 active-high |
| P02 | GPIO6 active-high |
| P03–P10 | MCP23017 GPA0–GPA7 active-high |

Referência código: `include/pin_map.h`, `include/hardware_config.h`, `docs/BOM.md`
