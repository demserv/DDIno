// @requirement RF-ENERGY-001 Leitura PZEM Modbus (V, A, W, kWh, PF, Hz)
// @requirement RF-ENERGY-006 Frequência da rede via PZEM
#include "driver_pzem.h"

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "pin_map.h"

static const char *TAG = "pzem";

#define PZEM_UART       UART_NUM_1
#define PZEM_BAUD       9600
#define PZEM_BUF_SIZE   256
#define PZEM_RX_TIMEOUT pdMS_TO_TICKS(200)

#define PZEM_ADDR       0xF8

static const uint8_t CMD_READ_ALL[8] = { PZEM_ADDR, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00 };
static const uint8_t CMD_RESET_ENERGY[8] = { PZEM_ADDR, 0x42, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 };

static uint16_t calc_crc(const uint8_t *buf, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void append_crc(uint8_t *buf, uint8_t len)
{
    uint16_t crc = calc_crc(buf, len - 2);
    buf[len - 2] = crc & 0xFF;
    buf[len - 1] = (crc >> 8) & 0xFF;
}

esp_err_t pzem_init(void)
{
    const uart_config_t uart_cfg = {
        .baud_rate = PZEM_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    esp_err_t err = uart_param_config(PZEM_UART, &uart_cfg);
    if (err != ESP_OK) return err;
    err = uart_set_pin(PZEM_UART, PIN_PZEM_TX_GPIO, PIN_PZEM_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;
    err = uart_driver_install(PZEM_UART, PZEM_BUF_SIZE, 0, 0, NULL, 0);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "PZEM-004T v4.0 init (UART1, 9600 8N1)");
    return ESP_OK;
}

esp_err_t pzem_read_all(pzem_data_t *data)
{
    if (!data) return ESP_ERR_INVALID_ARG;
    memset(data, 0, sizeof(*data));

    uint8_t cmd[8];
    memcpy(cmd, CMD_READ_ALL, 8);
    append_crc(cmd, 8);

    int written = uart_write_bytes(PZEM_UART, cmd, 8);
    if (written < 0) return ESP_ERR_INVALID_RESPONSE;

    uint8_t resp[25];
    int len = uart_read_bytes(PZEM_UART, resp, 25, PZEM_RX_TIMEOUT);
    if (len < 25) {
        data->valid = false;
        return ESP_ERR_TIMEOUT;
    }

    uint16_t crc_expected = (uint16_t)resp[23] | ((uint16_t)resp[24] << 8);
    uint16_t crc_calc = calc_crc(resp, 23);
    if (crc_calc != crc_expected)
    {
        data->valid = false;
        return ESP_ERR_INVALID_CRC;
    }

    data->voltage_v = (float)((resp[3] << 8) | resp[4]) / 10.0f;
    data->current_a = (float)((resp[5] << 8) | resp[6]) / 1000.0f;
    data->power_w = (float)((resp[7] << 8) | resp[8] | (resp[9] << 16) | (resp[10] << 24)) / 10.0f;
    data->energy_wh = (float)((resp[11] << 8) | resp[12] | (resp[13] << 16) | (resp[14] << 24)) / 1000.0f;
    data->frequency_hz = (float)((resp[15] << 8) | resp[16]) / 10.0f;
    data->pf = (float)((resp[17] << 8) | resp[18]) / 100.0f;
    data->valid = true;
    data->last_read_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);

    return ESP_OK;
}

esp_err_t pzem_reset_energy(void)
{
    uint8_t cmd[8];
    memcpy(cmd, CMD_RESET_ENERGY, 8);
    append_crc(cmd, 8);

    int written = uart_write_bytes(PZEM_UART, cmd, 8);
    if (written < 0) return ESP_ERR_INVALID_RESPONSE;

    return ESP_OK;
}

