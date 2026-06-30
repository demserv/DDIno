#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_timer.h"
#include "plug_model.h"
#include "hardware_config.h"
#include "global_state.h"
#include "core/circuit_breaker.h"
#include "drivers/driver_ds18b20.h"
#include "drivers/driver_pzem.h"
#include "drivers/driver_acs712.h"
#include "drivers/driver_mcp3208.h"
#include "pin_map.h"

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

typedef struct {
    float   temp_c;
    bool    temp_valid;
    int32_t ato_level_adc;
    bool    ato_valid;
    float   plug_currents_a[PLUG_COUNT_TOTAL];
    float   pzem_voltage_v;
    float   pzem_current_a;
    float   pzem_power_w;
    float   pzem_energy_wh;
    bool    pzem_valid;
    uint64_t timestamp_ms;
} sensor_snapshot_t;

esp_err_t api_sensors_take_snapshot(sensor_snapshot_t *snap)
{
    if (!snap) return ESP_ERR_INVALID_ARG;

    memset(snap, 0, sizeof(*snap));
    snap->timestamp_ms = esp_timer_get_time() / USEC_PER_MSEC;

    snap->temp_valid = circuit_breaker_is_available(CB_BUS_DS18B20);
    if (snap->temp_valid) {
        snap->temp_valid = ds18b20_read(&snap->temp_c);
    }

    snap->ato_valid = circuit_breaker_is_available(CB_BUS_SPI_ADC);
    if (snap->ato_valid) {
        uint16_t adc = 0;
        esp_err_t err = mcp3208_read_channel(PIN_ADC2_CS_GPIO, MCP3208_CH_ATO, &adc);
        snap->ato_valid = (err == ESP_OK);
        snap->ato_level_adc = snap->ato_valid ? (int32_t)adc : 0;
    }

    snap->pzem_valid = g_pzem.valid;
    if (snap->pzem_valid) {
        snap->pzem_voltage_v = g_pzem.voltage_v;
        snap->pzem_current_a = g_pzem.current_a;
        snap->pzem_power_w = g_pzem.power_w;
        snap->pzem_energy_wh = g_pzem.energy_wh;
    }

    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        acs712_read_plug(i + 1, &snap->plug_currents_a[i]);
    }

    return ESP_OK;
}
