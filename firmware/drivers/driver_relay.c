// @requirement RF-PLUG-001 Modos de operação por plugue
// @requirement RF-PLUG-009 Religamento sequencial
#include "driver_relay.h"
#include "driver/gpio.h"
#include "driver_mcp23017.h"
#include "hardware_config.h"
#include "pin_map.h"
#include "esp_log.h"

static uint16_t s_relay_state = 0;
static bool s_mcp23017_ok = false;

static inline int relay_level(bool on)
{
    return on ? HW_RELAY_ACTIVE_LEVEL : HW_RELAY_SAFE_OFF_LEVEL;
}

bool relay_mcp23017_ok(void)
{
    return s_mcp23017_ok;
}

esp_err_t relay_init_safe(void)
{
    s_relay_state = 0;
    s_mcp23017_ok = false;
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << PIN_RELAY_P01_GPIO) | (1ULL << PIN_RELAY_P02_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
    ESP_ERROR_CHECK(gpio_set_level(PIN_RELAY_P01_GPIO, relay_level(false)));
    ESP_ERROR_CHECK(gpio_set_level(PIN_RELAY_P02_GPIO, relay_level(false)));
    esp_err_t mcp_err = mcp23017_init(I2C_NUM_0, HW_I2C_ADDR_MCP23017);
    if (mcp_err != ESP_OK) {
        ESP_LOGE("relay", "MCP23017 nao detectado! Reles P03-P10 indisponiveis");
        return ESP_FAIL;
    }
    s_mcp23017_ok = true;
    ESP_ERROR_CHECK(mcp23017_set_relay_p03_p10_mask(0x00));
    return ESP_OK;
}

esp_err_t relay_all_off(void)
{
    s_relay_state = 0;
    ESP_ERROR_CHECK(gpio_set_level(PIN_RELAY_P01_GPIO, relay_level(false)));
    ESP_ERROR_CHECK(gpio_set_level(PIN_RELAY_P02_GPIO, relay_level(false)));
    if (!s_mcp23017_ok) return ESP_FAIL;
    return mcp23017_set_relay_p03_p10_mask(0x00);
}

esp_err_t relay_set_p01(bool on)
{
    esp_err_t err = gpio_set_level(PIN_RELAY_P01_GPIO, relay_level(on));
    if (err == ESP_OK)
    {
        if (on) s_relay_state |=  (1U << 0);
        else    s_relay_state &= ~(1U << 0);
    }
    return err;
}

esp_err_t relay_set_p02(bool on)
{
    esp_err_t err = gpio_set_level(PIN_RELAY_P02_GPIO, relay_level(on));
    if (err == ESP_OK)
    {
        if (on) s_relay_state |=  (1U << 1);
        else    s_relay_state &= ~(1U << 1);
    }
    return err;
}

esp_err_t relay_set_p03_p10_mask(uint8_t on_mask)
{
    if (!s_mcp23017_ok) return ESP_FAIL;
    uint16_t mask16 = on_mask;
    s_relay_state = (s_relay_state & 0x0003) | (mask16 << 2);
    return mcp23017_set_relay_p03_p10_mask(on_mask);
}

esp_err_t relay_set(uint8_t plug_id, bool on)
{
    if (plug_id < 1 || plug_id > 10) return ESP_ERR_INVALID_ARG;
    if (plug_id == 1) return relay_set_p01(on);
    if (plug_id == 2) return relay_set_p02(on);
    uint8_t bit = plug_id - 3;
    uint8_t mask = s_relay_state >> 2;
    if (on) mask |= (1U << bit);
    else    mask &= ~(1U << bit);
    return relay_set_p03_p10_mask(mask);
}

bool relay_get(uint8_t plug_id)
{
    if (plug_id < 1 || plug_id > 10) return false;
    uint8_t bit = plug_id - 1;
    return (s_relay_state >> bit) & 1U;
}
