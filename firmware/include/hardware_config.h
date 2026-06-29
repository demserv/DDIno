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

// --- I2C bus ---
#define HW_I2C_ADDR_MCP23017             (0x20U)
#define HW_I2C_ADDR_DS3231               (0x68U)
#define HW_I2C_CLK_HZ                    (100000U)
#define HW_I2C_TIMEOUT_MS                (100U)

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

// --- UART / PZEM ---
#define HW_PZEM_MODEL                    "PZEM-004T-v4"
#define HW_UART_BAUD                    (9600U)
#define HW_UART_BUF_SIZE                (1024U)

// --- SPI ---
#define HW_SPI_CLK_TFT_HZ                (20000000U)
#define HW_SPI_CLK_SD_HZ                 (10000000U)
#define HW_SPI_CLK_TOUCH_HZ              (2000000U)
#define HW_SPI_CLK_MCP3208_HZ            (1000000U)
#define HW_SPI_MAX_TRANSFER_SZ           (4096U)
#define HW_SPI_QUEUE_DEPTH              (1U)
#define HW_SPI_MUTEX_TIMEOUT_MS          (100U)

// --- NVS ---
#define HW_NVS_ERASE_IF_CORRUPT          (1U)

// --- WDT ---
#define HW_WDT_TIMEOUT_MS                (2000U)
#define HW_WDT_ADV_TIMEOUT_MS            (10000U)

// --- SD card ---
#define HW_SD_SPI_HOST                   (SPI2_HOST)
#define HW_SD_MOUNT_POINT                "/sdcard"
#define HW_SD_MAX_FILES                  (8U)
#define HW_SD_ALLOC_UNIT                 (16384U)

// --- HTTP ---
#define HW_HTTP_PORT                     (80U)
#define HW_HTTP_MAX_URI_HANDLERS         (32U)

// --- LVGL / Display ---
#define HW_UI_LVGL_TASK_STACK            (4096U)
#define HW_UI_LVGL_TASK_PRIORITY         (5U)
#define HW_UI_LVGL_TICK_MS               (5U)
#define HW_DISP_HOR_RES                 (480U)
#define HW_DISP_VER_RES                 (320U)
#define HW_DISP_RESET_PULSE_MS           (10U)
#define HW_DISP_RESET_WAIT_MS           (120U)

// --- Safety timing (compile-time safety bounds, not user-configurable) ---
#define HW_SAFEOFF_CAUSE_STABLE_S        (10U)
#define HW_EMERGENCY_CAUSE_STABLE_S      (30U)
#define HW_ANTIFLAP_COOLDOWN_MS          (3000U)
#define HW_ANTIFLAP_WINDOW_MS            (60000U)
#define HW_ANTIFLAP_MAX_TRANSITIONS      (3U)

// --- Restart / reset ---
#define HW_RESTART_COUNTDOWN_DEFAULT_S   (10U)
#define HW_RESTART_CONFIRM_WINDOW_S      (10U)
#define HW_RESTART_WAIT_MS              (30000U)
#define HW_RESTART_STAGGER_MS           (5000U)
#define HW_RESTART_MONITOR_MS          (10000U)

// --- UI defaults (override via ConfigRoot) ---
#define HW_UI_BRIGHTNESS_DEFAULT         (100U)
#define HW_UI_CAROUSEL_INTERVAL_MS       (15000U)
#define HW_UI_CAROUSEL_PAUSE_ON_ACTIVITY_MS (5000U)

// --- Time conversion helpers ---
#define SECONDS_PER_MINUTE              (60U)
#define MINUTES_PER_HOUR                (60U)
#define SECONDS_PER_HOUR                (3600U)
#define MS_PER_SECOND                   (1000U)
#define MS_PER_HOUR                     (3600000ULL)

// --- Keypad ADC thresholds (via MCP3208 CH3) ---
#define HW_AD_KEYPAD_THRESH_NONE        (4000U)
#define HW_AD_KEYPAD_THRESH_FEED        (3600U)
#define HW_AD_KEYPAD_THRESH_MENU        (3000U)
#define HW_AD_KEYPAD_THRESH_ESC         (2400U)
#define HW_AD_KEYPAD_THRESH_ENTER       (1800U)
#define HW_AD_KEYPAD_THRESH_RIGHT       (1200U)
#define HW_AD_KEYPAD_THRESH_LEFT        (600U)
#define HW_AD_KEYPAD_THRESH_DOWN        (100U)

// --- ATO FSM ---
#define HW_ATO_DEBOUNCE_COUNT           (3U)
#define HW_ATO_ADC_HYSTERESIS           (50U)

// --- Touch ---
#define HW_TOUCH_DEBOUNCE_COUNT         (2U)
#define HW_TOUCH_SPI_BUF_SIZE           (3U)

// --- Logging ---
#define HW_SD_LOG_INTERVAL_S             (300U)
#define HW_SD_PATH_BUF_SIZE             (320U)
#define HW_SD_CMD_BUF_SIZE              (64U)

#endif
