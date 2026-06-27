// @requirement RF-PLUG-001 Controle de relés P03-P10 via MCP23017 + ULN2803
#ifndef DRIVER_MCP23017_H
#define DRIVER_MCP23017_H

#include <stdint.h>
#include "driver/i2c.h"
#include "esp_err.h"

esp_err_t mcp23017_init(i2c_port_t port, uint8_t addr);
esp_err_t mcp23017_write_gpioa(uint8_t value);
esp_err_t mcp23017_write_gpiob(uint8_t value);
esp_err_t mcp23017_set_relay_p03_p10_mask(uint8_t on_mask);
esp_err_t mcp23017_read_port(uint8_t *gpioa, uint8_t *gpiob);

#endif
