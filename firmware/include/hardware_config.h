// @requirement RNF-HARDWARE-001 Constantes físicas e de hardware (NÃO confundir com parâmetros operacionais)
#ifndef FIRMWARE_INCLUDE_HARDWARE_CONFIG_H
#define FIRMWARE_INCLUDE_HARDWARE_CONFIG_H

#include <stdint.h>

// ============================================================
// hardware_config.h — Hardware invariants
// ============================================================
// GPIOs are in pin_map.h
// Operational defaults are in param_catalog.h / ConfigRoot
// ============================================================

#define HW_PLATFORM_NAME                 "ESP32-S3-DevKitC-1 N16R8"
#define HW_FW_TARGET                     "esp32s3"

// --- I2C addresses ---
#define HW_I2C_ADDR_MCP23017             (0x20U)
#define HW_I2C_ADDR_DS3231               (0x68U)

// --- Feature presence ---
#define HW_FEATURE_RTC_DS3231_PRESENT    (1U)
#define HW_FEATURE_BUZZER_PRESENT        (1U)
#define HW_FEATURE_MONITOR_ONLY_MODE     (1U)

// --- Relay characteristics ---
#define HW_RELAY_P01_ACTIVE_LEVEL        (1U)
#define HW_RELAY_P02_ACTIVE_LEVEL        (1U)
#define HW_RELAY_P03_P10_ACTIVE_LEVEL    (1U)
#define HW_RELAY_SAFE_OFF_LEVEL          (0U)
#define HW_RELAY_COUNT_MAX               (10U)
#define HW_RELAY_MODULE_INPUT_VOLTAGE_V  (5U)
#define HW_RELAY_MODULE_HAS_OPTO         (1U)
#define HW_RELAY_MODULE_HAS_JD_VCC       (1U)

// --- ADC characteristics ---
#define HW_ADC_VREF_MV                   (3300U)
#define HW_ADC_MAX_COUNT                 (4095U)

// --- ACS712 ---
#define HW_ACS712_MODEL_20A              (1U)
#define HW_ACS712_SUPPLY_MV              (5000U)
#define HW_ACS712_CONDITIONED_TO_3V3     (1U)

// --- ATO sensor ---
#define HW_ATO_SIGNAL_MODE_DIGITAL_ON_OFF_ADC (1U)
#define HW_ATO_SENSOR_SUPPLY_MV          (5000U)

// --- Keypad ---
#define HW_AD_KEYPAD_SUPPLY_MV           (5000U)
#define HW_AD_KEYPAD_ADC_CONDITIONED     (1U)
#define HW_AD_KEYPAD_ADC_UP_THRESH_MIN   (100U)
#define HW_AD_KEYPAD_ADC_UP_THRESH_MAX   (600U)

// --- Environmental ---
#define HW_AC_PROTOBOARD_FORBIDDEN       (1U)
#define HW_AC_DUPONT_FORBIDDEN           (1U)
#define HW_AC_PLASTIC_ENCLOSURE          (1U)
#define HW_MAINS_BIVOLT_SUPPORTED        (1U)
#define HW_AC_MAX_CURRENT_PER_OUTLET_A   (10U)
#define HW_AC_MAIN_INPUT_FUSE            (1U)
#define HW_AC_FUSE_PER_OUTLET            (1U)
#define HW_BUCK_3V3_REQUIRED             (1U)
#define HW_BUCK_5V_REQUIRED              (1U)

// --- DS18B20 ---
#define HW_TEMP_SENSOR_COUNT             (1U)
#define HW_DS18B20_RESOLUTION            (12U)
#define HW_DS18B20_CONVERSION_MS         (750U)
#define HW_DS18B20_CRC_REQUIRED          (1U)
#define HW_DS18B20_REJECT_85C            (1U)

// --- RTC ---
#define HW_DS3231_BACKUP_BATTERY         (1U)

// --- PZEM ---
#define HW_PZEM_MODEL                    "PZEM-004T-v4"

// --- SPI clocks ---
#define HW_SPI_CLK_TFT_HZ                (20000000U)
#define HW_SPI_CLK_SD_HZ                 (10000000U)
#define HW_SPI_CLK_TOUCH_HZ              (2000000U)
#define HW_SPI_CLK_MCP3208_HZ            (1000000U)

// --- NVS ---
#define HW_NVS_ERASE_IF_CORRUPT          (1U)

// --- WDT ---
#define HW_WDT_TIMEOUT_MS                (2000U)

// --- SD card ---
#define HW_SD_SPI_HOST                   (SPI2_HOST)
#define HW_SD_MOUNT_POINT                "/sdcard"
#define HW_SD_MAX_FILES                  (8U)
#define HW_SD_ALLOC_UNIT                 (16384U)

// --- HTTP ---
#define HW_HTTP_PORT                     (80U)
#define HW_HTTP_MAX_URI_HANDLERS         (32U)

// --- LVGL ---
#define HW_UI_LVGL_TASK_STACK            (4096U)
#define HW_UI_LVGL_TASK_PRIORITY         (5U)
#define HW_UI_LVGL_TICK_MS               (5U)

// --- Safety timing (compile-time safety bounds, not user-configurable) ---
#define HW_SAFEOFF_CAUSE_STABLE_S        (10U)
#define HW_EMERGENCY_CAUSE_STABLE_S      (30U)
#define HW_ANTIFLAP_COOLDOWN_MS          (3000U)
#define HW_ANTIFLAP_WINDOW_MS            (60000U)
#define HW_ANTIFLAP_MAX_TRANSITIONS      (3U)

// --- Restart / reset ---
#define HW_RESTART_COUNTDOWN_DEFAULT_S   (10U)
#define HW_RESTART_CONFIRM_WINDOW_S      (10U)

// --- UI defaults (override via ConfigRoot) ---
#define HW_UI_BRIGHTNESS_DEFAULT         (100U)
#define HW_UI_CAROUSEL_INTERVAL_MS       (15000U)
#define HW_UI_CAROUSEL_PAUSE_ON_ACTIVITY_MS (5000U)

// --- Version strings ---
#define HW_FW_VERSION_STR                "F1.0.0"
#define HW_SRS_VERSION_STR               "v3.11-AF.3+AF.4+B12/N+B13/N"
#define HW_CONFIG_SCHEMA_VERSION_STR     "1.0"

// --- Health check ---
#define HW_HEALTH_CHECK_INTERVAL_S       (60U)

// --- Heartbeat check cycle interval ---
#define HW_HEARTBEAT_CHECK_CYCLE_INTERVAL (20U)

// --- Temperature filter window ---
#define HW_TEMP_FILTER_WINDOW            (3U)

// --- Default sensor values ---
#define HW_TEMP_DEFAULT_C                (25.0f)
#define HW_ATO_DEFAULT_ADC               (0)

// --- Grid frequency ---
#define HW_MAINS_FREQUENCY_HZ            (50U)

// --- Factory reset hold time ---
#define HW_RESET_HOLD_MS                 (10000U)

// --- Feed snapshot interval ---
#define HW_FEED_SNAPSHOT_INTERVAL_MS     (5000U)

// --- LED blink interval ---
#define HW_LED_BLINK_INTERVAL_MS         (500U)

// --- Safety exit stabilization time ---
#define HW_SAFE_EXIT_STABLE_S            (5U)

// --- Logging ---
#define HW_SD_LOG_INTERVAL_S             (300U)

#endif
