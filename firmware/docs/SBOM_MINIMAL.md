# SBOM — Software Bill of Materials (Minimal)

| Field | Value |
|-------|-------|
| Document Version | 1.0 |
| Date | 2026-06-27 |
| Firmware Version | compliance-fix-v1 |
| SRS Baseline | Técnico Consolidado Final v3.11-AF.3 + AF.4 + Bloco 12/N + Bloco 13/N |

## Project-Authored Components

| Component | Language | Files | Lines (approx) | Description |
|-----------|----------|-------|----------------|-------------|
| App Main | C | 1 | 931 | Entry point, boot sequence, task loop |
| Core | C | 6 | ~800 | Global state, safety controller, circuit breaker, event bus |
| Drivers | C | 10 | ~1500 | PZEM, DS18B20, DS3231, ACS712, MCP23017, MCP3208, relay, keypad, buzzer/LED |
| FSM | C | 6 | ~1200 | Thermal, ATO, feed, restart state machines |
| HAL | C | 2 | ~400 | I2C and SPI bus abstraction + mutex |
| Include | H | 20+ | ~800 | Hardware config, pin map, param catalog, system types |
| Security | C | 1 | ~200 | Security policy (fuse, anti-rollback stubs) |
| Services | C | 25+ | ~4000 | Config manager, alert manager, plug manager, storage, WDT, health matrix, etc. |
| UI (legacy) | C | 10+ | ~2000 | Legacy screen system, display driver, touch, keypad |
| UI (HMI) | C | 15+ | ~2500 | LVGL-based HMI: screens, components, view model, events, carousel |
| Web | C | 3 | ~1500 | REST API, auth, rate limiting |
| Docs | MD | 20+ | ~1500 | Architecture, RTM, test plan, compliance, decisions, ownership |

## Third-Party Components

| Component | Version | Supplier | License | Integration Method |
|-----------|---------|----------|---------|-------------------|
| LVGL | ^8.3 | LVGL LLC | MIT | IDF managed component |
| ESP-IDF | >=4.1.0 | Espressif | Apache 2.0 | System toolchain |
| FreeRTOS | (in IDF) | Amazon/Espressif | MIT | Bundled with ESP-IDF |
| Unity | (in IDF) | ThrowTheSwitch | MIT | ESP-IDF test framework |
| CMock | (in IDF) | ThrowTheSwitch | MIT | ESP-IDF test framework |

## Dependency Tree

```
Monitor Aquário Inteligente
├── Project code (all .c/.h under firmware/)
│   ├── main/app_main.c
│   ├── core/*
│   ├── drivers/*
│   ├── fsm/*
│   ├── hal/*
│   ├── include/*
│   ├── security/*
│   ├── services/*
│   ├── ui/*
│   ├── web/*
│   └── docs/*
├── LVGL v8.3 (graphics)
└── ESP-IDF (framework)
    ├── FreeRTOS
    ├── esp_http_server
    ├── nvs_flash
    ├── driver/*
    ├── mbedtls
    └── ...
```

## Notes

- This SBOM is "minimal" — it lists only components relevant to the firmware deliverable.
- Full component manifests (including transitive dependencies) are maintained by the ESP-IDF component manager and LVGL's own `idf_component.yml`.
- All project code is written in C11 (C99 compatible subset) for ESP32-S3 target.
