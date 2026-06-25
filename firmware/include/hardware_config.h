#ifndef FIRMWARE_INCLUDE_HARDWARE_CONFIG_H
#define FIRMWARE_INCLUDE_HARDWARE_CONFIG_H

#include <stdint.h>

#define HW_PLATFORM_NAME                 "ESP32-S3-DevKitC-1 N16R8"
#define HW_FW_TARGET                     "esp32s3"
#define HW_I2C_ADDR_MCP23017             (0x20U)
#define HW_I2C_ADDR_DS3231               (0x68U)
#define HW_RELAY_ACTIVE_LEVEL            (1U)
#define HW_RELAY_SAFE_OFF_LEVEL          (0U)
#define HW_ATO_MODEL_DIGITAL_ON_OFF_ADC  (1U)
#define HW_ACS712_MODEL_20A              (1U)
#define HW_MAINS_BIVOLT_SUPPORTED        (1U)
#define HW_PLUG_MAX_CURRENT_A            (10U)
#define HW_ADC_INPUT_MAX_V               (3.3f)
#define HW_KEYPAD_CONDITIONED_TO_3V3     (1U)
#define HW_ACS712_CONDITIONED_TO_3V3     (1U)
#define HW_AC_PROTOBOARD_FORBIDDEN       (1U)
#define HW_BUCK_3V3_REQUIRED             (1U)

#endif
