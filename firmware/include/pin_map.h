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

/* ============================================================
 * Baseline normativa de pinagem (AF.3 / GO CODING RevA)
 * Nomes canônicos exigidos pela SRS. Valores idênticos aos
 * existentes acima; aliases adicionados sem remover legados.
 * ============================================================ */
#ifndef PIN_UNUSED
#define PIN_UNUSED                  (-1)
#endif

#ifndef PIN_DS18B20_1WIRE_GPIO
#define PIN_DS18B20_1WIRE_GPIO      4
#endif
#ifndef PIN_PZEM_UART_TX_GPIO
#define PIN_PZEM_UART_TX_GPIO       17
#endif
#ifndef PIN_PZEM_UART_RX_GPIO
#define PIN_PZEM_UART_RX_GPIO       18
#endif

#ifndef MCP23017_I2C_ADDR
#define MCP23017_I2C_ADDR           0x20
#endif

#define MCP23017_P03_PORT           'A'
#define MCP23017_P04_PORT           'A'
#define MCP23017_P05_PORT           'A'
#define MCP23017_P06_PORT           'A'
#define MCP23017_P07_PORT           'A'
#define MCP23017_P08_PORT           'A'
#define MCP23017_P09_PORT           'A'
#define MCP23017_P10_PORT           'A'

#define MCP23017_P03_BIT            0
#define MCP23017_P04_BIT            1
#define MCP23017_P05_BIT            2
#define MCP23017_P06_BIT            3
#define MCP23017_P07_BIT            4
#define MCP23017_P08_BIT            5
#define MCP23017_P09_BIT            6
#define MCP23017_P10_BIT            7

#define MCP23017_LED_GREEN_PORT     'B'
#define MCP23017_LED_YELLOW_PORT    'B'
#define MCP23017_LED_RED_PORT       'B'
#define MCP23017_BUZZER_PORT        'B'

#define MCP23017_LED_GREEN_BIT      2
#define MCP23017_LED_YELLOW_BIT     3
#define MCP23017_LED_RED_BIT        4
#define MCP23017_BUZZER_BIT         5

#define ADC1_CH_ACS_P01             0
#define ADC1_CH_ACS_P02             1
#define ADC1_CH_ACS_P03             2
#define ADC1_CH_ACS_P04             3
#define ADC1_CH_ACS_P05             4
#define ADC1_CH_ACS_P06             5
#define ADC1_CH_ACS_P07             6
#define ADC1_CH_ACS_P08             7

#define ADC2_CH_ACS_P09             0
#define ADC2_CH_ACS_P10             1
#define ADC2_CH_ATO_LEVEL           2
#define ADC2_CH_AD_KEYPAD           3
#define ADC2_CH_RESERVED_4          4
#define ADC2_CH_RESERVED_5          5
#define ADC2_CH_RESERVED_6          6
#define ADC2_CH_RESERVED_7          7

#endif
