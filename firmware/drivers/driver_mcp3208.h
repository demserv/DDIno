#ifndef DRIVER_MCP3208_H
#define DRIVER_MCP3208_H

#include <stdint.h>
#include "esp_err.h"

#define MCP3208_CH_P01_ACS  0
#define MCP3208_CH_P02_ACS  1
#define MCP3208_CH_P03_ACS  2
#define MCP3208_CH_P04_ACS  3
#define MCP3208_CH_P05_ACS  4
#define MCP3208_CH_P06_ACS  5
#define MCP3208_CH_P07_ACS  6
#define MCP3208_CH_P08_ACS  7
#define MCP3208_CH_P09_ACS  0
#define MCP3208_CH_P10_ACS  1
#define MCP3208_CH_ATO      2
#define MCP3208_CH_KEYPAD   3

esp_err_t mcp3208_init(int cs_gpio);
esp_err_t mcp3208_read_channel(int cs_gpio, uint8_t channel, uint16_t *adc_value);

#endif
