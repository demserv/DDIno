/* @requirement RF-HW-PIN-001 a RF-HW-PIN-046 GO CODING RevA */
// @requirement RNF-HARDWARE-001 Pinagem canônica AF.3 — GPIO, I2C, SPI, UART, ADC
#ifndef FIRMWARE_INCLUDE_PIN_MAP_H
#define FIRMWARE_INCLUDE_PIN_MAP_H

#define PIN_DS18B20_DQ_GPIO      (4)
#define PIN_RELAY_P01_GPIO       (5)
#define PIN_RELAY_P02_GPIO       (6)
#define PIN_I2C_SDA_GPIO         (8)
#define PIN_I2C_SCL_GPIO         (9)
#define PIN_TFT_CS_GPIO          (10)
#define PIN_SPI_MOSI_GPIO        (11)
#define PIN_SPI_MISO_GPIO        (12)
#define PIN_SPI_SCK_GPIO         (13)
#define PIN_SD_CS_GPIO           (15)
#define PIN_TFT_DC_GPIO          (16)
#define PIN_PZEM_TX_GPIO         (17)
#define PIN_PZEM_RX_GPIO         (18)
#define PIN_TFT_RST_GPIO         (21)
#define PIN_TOUCH_CS_GPIO        (38)
#define PIN_TOUCH_IRQ_GPIO       (39)
#define PIN_ADC1_CS_GPIO         (41)
#define PIN_ADC2_CS_GPIO         (42)
#define PIN_TFT_BL_GPIO          (47)

#define MCP_P03_GPA_BIT          (0)
#define MCP_P04_GPA_BIT          (1)
#define MCP_P05_GPA_BIT          (2)
#define MCP_P06_GPA_BIT          (3)
#define MCP_P07_GPA_BIT          (4)
#define MCP_P08_GPA_BIT          (5)
#define MCP_P09_GPA_BIT          (6)
#define MCP_P10_GPA_BIT          (7)
#define MCP_LED_GREEN_GPB_BIT    (2)
#define MCP_LED_YELLOW_GPB_BIT   (3)
#define MCP_LED_RED_GPB_BIT      (4)
#define MCP_BUZZER_GPB_BIT       (5)

#endif
