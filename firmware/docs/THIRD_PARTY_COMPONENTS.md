# Third-Party Components — Monitor Aquário Inteligente

## Version

| Field | Value |
|-------|-------|
| Document Version | 1.0 |
| Date | 2026-06-27 |
| Firmware Version | compliance-fix-v1 |

## Component Inventory

| Component | Version | License | Source | Purpose | Location |
|-----------|---------|---------|--------|---------|----------|
| LVGL | ^8.3 | MIT | Managed component (`lvgl/lvgl`) | Graphics library for ILI9488 TFT display | `managed_components/lvgl__lvgl/` |
| ESP-IDF | >=4.1.0 | Apache 2.0 | Espressif | IoT Development Framework | System-wide |
| FreeRTOS | (included in ESP-IDF) | MIT | Espressif (Amazon) | Real-time OS kernel | (included in ESP-IDF) |
| Unity | (included in ESP-IDF test framework) | MIT | ThrowTheSwitch | Unit test framework | `test/` (via ESP-IDF) |
| CMock | (included in ESP-IDF test framework) | MIT | ThrowTheSwitch | Mock generation for tests | `test/` (via ESP-IDF) |

## Component Details

### LVGL v8.3

- **managed_component.yml reference**: `lvgl/lvgl: "^8.3"`
- **Actual installed version**: As resolved by `idf.py` component manager
- **License**: MIT
- **Integration**: Standard ESP-IDF managed component via `idf_component.yml`
- **Kconfig**: `CONFIG_LVGL_USE_ILI9488=y`, `CONFIG_LV_DPI=96`
- **Modified files**: None (stock component, not patched)
- **Audit status**: ⚠️ Component includes `docs/`, `examples/`, `demos/`, `tests/` directories that are NOT part of the project source and are excluded from audit scope

### ESP-IDF

- **Minimum version**: v4.1.0 (declared in `main/idf_component.yml`)
- **Expected version**: v5.x (based on API usage patterns)
- **License**: Apache 2.0
- **Key subsystems used**:
  - `esp_http_server` — REST API
  - `esp_spiffs` — SPIFFS filesystem for storage/logs partitions
  - `esp_ota_ops` — OTA update support (deferred)
  - `esp_wifi` — Wi-Fi connectivity
  - `driver/gpio` — GPIO control
  - `driver/spi_master` — SPI bus
  - `driver/i2c` — I2C bus
  - `driver/uart` — UART for PZEM
  - `driver/adc` — ADC (via MCP3208 external)
  - `nvs_flash` — NVS for config storage
  - `mbedtls` — SHA-256 for auth

### FreeRTOS

- **Version**: As included in the ESP-IDF version used
- **License**: MIT
- **Key features used**:
  - Tasks (via `xTaskCreatePinnedToCore`)
  - Mutexes (via `xSemaphoreCreateMutex`)
  - Queues (via `xQueueCreate`)
  - Timers (via `xTimerCreate`)
  - Task notifications (via `xTaskNotifyGive`)
  - Watchdog (via `esp_task_wdt_add`)

## Dependency Graph

```
Monitor Aquário Inteligente
├── ESP-IDF (system framework)
│   ├── FreeRTOS (kernel)
│   ├── esp_http_server (web server)
│   ├── esp_spiffs (filesystem)
│   ├── esp_ota_ops (OTA)
│   ├── esp_wifi (networking)
│   ├── driver/* (peripheral drivers)
│   ├── nvs_flash (config storage)
│   └── mbedtls (crypto)
├── LVGL v8.3 (graphics)
│   └── ILI9488 display driver
└── Test framework
    ├── Unity (test runner)
    └── CMock (mocks)
```

## Compliance Notes

- All third-party components are used as-is without modification
- No patches or local forks are maintained
- Licenses are compatible with the project's distribution model (closed-source embedded firmware on physical hardware)
- Full license texts are available in the respective component directories
- LVGL docs/examples/demos/tests directories are excluded from source code concatenation and audit scope per AUDIT_SCOPE.md
