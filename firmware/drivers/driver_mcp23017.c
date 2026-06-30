// @requirement RF-PLUG-001 Controle de relés P03-P10 via MCP23017 + ULN2803
// @requirement RNF-HARDWARE-001 I2C GPIO expander @0x20
#include "driver_mcp23017.h"
#include "hardware_config.h"
#include "hal/hal_bus.h"
#include "core/circuit_breaker.h"
#include "esp_log.h"

#define MCP23017_REG_IODIRA 0x00
#define MCP23017_REG_IODIRB 0x01
#define MCP23017_REG_IOCON  0x0A
#define MCP23017_REG_GPIOA  0x12
#define MCP23017_REG_GPIOB  0x13
#define MCP23017_REG_OLATA  0x14
#define MCP23017_REG_OLATB  0x15

static const char *TAG = "mcp23017";
static i2c_port_t s_port = I2C_NUM_0;
static uint8_t s_addr = 0x20;

static esp_err_t mcp_write_reg(uint8_t reg, uint8_t val)
{
    if (!circuit_breaker_is_available(CB_BUS_I2C)) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t data[2] = { reg, val };
    esp_err_t err = hal_i2c_master_write_to_device(s_port, s_addr, data, sizeof(data),
                                                   pdMS_TO_TICKS(HW_I2C_TIMEOUT_MS));
    if (err == ESP_OK) {
        circuit_breaker_record_success(CB_BUS_I2C);
    } else {
        circuit_breaker_record_failure(CB_BUS_I2C);
    }
    return err;
}

static esp_err_t mcp_read_reg(uint8_t reg, uint8_t *val)
{
    if (!circuit_breaker_is_available(CB_BUS_I2C)) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = hal_i2c_master_write_read_device(s_port, s_addr, &reg, 1, val, 1,
                                                     pdMS_TO_TICKS(HW_I2C_TIMEOUT_MS));
    if (err == ESP_OK) {
        circuit_breaker_record_success(CB_BUS_I2C);
    } else {
        circuit_breaker_record_failure(CB_BUS_I2C);
    }
    return err;
}

esp_err_t mcp23017_init(i2c_port_t port, uint8_t addr)
{
    s_port = port;
    s_addr = addr;

    esp_err_t err = mcp_write_reg(MCP23017_REG_IOCON, 0x00);
    if (err != ESP_OK) return err;

    err = mcp_write_reg(MCP23017_REG_IODIRA, 0x00);
    if (err != ESP_OK) return err;

    err = mcp_write_reg(MCP23017_REG_IODIRB, 0xC3);
    if (err != ESP_OK) return err;

    err = mcp_write_reg(MCP23017_REG_OLATA, 0x00);
    if (err != ESP_OK) return err;

    err = mcp_write_reg(MCP23017_REG_OLATB, 0x00);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "MCP23017 inicializado addr=0x%02X (safe OFF)", s_addr);
    return ESP_OK;
}

esp_err_t mcp23017_write_gpioa(uint8_t value)
{
    return mcp_write_reg(MCP23017_REG_GPIOA, value);
}

esp_err_t mcp23017_write_gpiob(uint8_t value)
{
    return mcp_write_reg(MCP23017_REG_GPIOB, value);
}

esp_err_t mcp23017_set_relay_p03_p10_mask(uint8_t on_mask)
{
    return mcp23017_write_gpioa(on_mask);
}

esp_err_t mcp23017_read_port(uint8_t *gpioa, uint8_t *gpiob)
{
    if (!gpioa && !gpiob) return ESP_ERR_INVALID_ARG;
    if (gpioa)
    {
        esp_err_t err = mcp_read_reg(MCP23017_REG_GPIOA, gpioa);
        if (err != ESP_OK) return err;
    }
    if (gpiob)
    {
        esp_err_t err = mcp_read_reg(MCP23017_REG_GPIOB, gpiob);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}
