#ifndef FIRMWARE_INCLUDE_HARDWARE_CONFIG_H
#define FIRMWARE_INCLUDE_HARDWARE_CONFIG_H

#include <stdint.h>

#define HW_PLATFORM_NAME                 "ESP32-S3-DevKitC-1 N16R8"
#define HW_FW_TARGET                     "esp32s3"

#define HW_I2C_ADDR_MCP23017             (0x20U)
#define HW_I2C_ADDR_DS3231               (0x68U)

#define HW_RELAY_ACTIVE_LEVEL            (1U)
#define HW_RELAY_SAFE_OFF_LEVEL          (0U)
#define HW_RELAY_COUNT_MAX               (10U)

#define HW_ATO_MODEL_DIGITAL_ON_OFF_ADC  (1U)
#define HW_ACS712_MODEL_20A              (1U)
#define HW_MAINS_BIVOLT_SUPPORTED        (1U)

#define HW_PLUG_MAX_CURRENT_A            (10U)
#define HW_PLUG_DEFAULT_CURRENT_LIMIT_A  (10.0f)
#define HW_PLUG_CRITICAL_P01             (1U)
#define HW_PLUG_CRITICAL_P02             (1U)
#define HW_PLUG_FATOR_CURTO_DEFAULT      (3.0f)
#define HW_PLUG_TEMPO_CURTO_MS_DEFAULT   (500U)

#define HW_ADC_INPUT_MAX_V               (3.3f)
#define HW_ADC_ATO_CHANNEL               (0U)
#define HW_ADC_KEYPAD_CHANNEL            (1U)
#define HW_ADC_CS_ATO_GPIO               (42)
#define HW_ADC_CS_KEYPAD_GPIO            (41)

#define HW_KEYPAD_CONDITIONED_TO_3V3     (1U)
#define HW_ACS712_CONDITIONED_TO_3V3     (1U)
#define HW_AC_PROTOBOARD_FORBIDDEN       (1U)
#define HW_BUCK_3V3_REQUIRED             (1U)
#define HW_BUCK_5V_REQUIRED              (1U)

#define HW_TEMP_SENSOR_COUNT             (1U)
#define HW_DS18B20_RESOLUTION            (12U)
#define HW_DS18B20_CONVERSION_MS         (750U)
#define HW_DS18B20_CRC_REQUIRED          (1U)
#define HW_DS18B20_REJECT_85C            (1U)

#define HW_DS3231_BACKUP_BATTERY         (1U)

#define HW_PZEM_MODEL                    "PZEM-004T-v3"
#define HW_PZEM_READ_TIMEOUT_MS          (1000U)
#define HW_PZEM_RETRY_COUNT              (3U)

#define HW_NVS_ERASE_IF_CORRUPT          (1U)

#define HW_WDT_TIMEOUT_MS                (2000U)
#define HW_TASK_MAIN_LOOP_PERIOD_MS      (50U)

#define HW_SD_SPI_HOST                   (SPI2_HOST)
#define HW_SD_MOUNT_POINT                "/sdcard"
#define HW_SD_MAX_FILES                  (8U)
#define HW_SD_ALLOC_UNIT                 (16384U)

#define HW_HTTP_PORT                     (80U)
#define HW_HTTP_MAX_URI_HANDLERS         (32U)

#define HW_UI_LVGL_TASK_STACK            (4096U)
#define HW_UI_LVGL_TASK_PRIORITY         (5U)
#define HW_UI_LVGL_TICK_MS               (5U)

#define HW_SENSOR_READ_INTERVAL_MS       (1000U)
#define HW_ENERGY_UPDATE_INTERVAL_S      (60U)
#define HW_HEALTH_CHECK_INTERVAL_S       (60U)
#define HW_SD_LOG_INTERVAL_S             (300U)
#define HW_NVS_COMMIT_INTERVAL_S         (60U)
#define HW_FEED_SNAPSHOT_INTERVAL_MS     (5000U)
#define HW_FEED_SNAPSHOT_TTL_S           (1800U)

#define HW_ANTIFLAP_COOLDOWN_MS          (3000U)
#define HW_ANTIFLAP_WINDOW_MS            (60000U)
#define HW_ANTIFLAP_MAX_TRANSITIONS      (3U)

#define HW_SAFEOFF_CAUSE_STABLE_S        (10U)
#define HW_EMERGENCY_CAUSE_STABLE_S      (30U)

#endif
