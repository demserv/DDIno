// @requirement RF-SENSOR-001 a RF-SENSOR-003 Medição coordenada de sensores
#include "sensor_ctl.h"
#include "driver_ds18b20.h"
#include "driver_acs712.h"
#include "driver_pzem.h"

static const char *s_names[SENSOR_TYPE_COUNT] = {
    "Temperature", "Current", "Power"
};
static int s_errors[SENSOR_TYPE_COUNT] = {0};

const char *sensor_ctl_get_name(sensor_ctl_type_t type)
{
    if ((int)type < 0 || type >= SENSOR_TYPE_COUNT) return "Unknown";
    return s_names[type];
}

esp_err_t sensor_ctl_init(void)
{
    for (int i = 0; i < SENSOR_TYPE_COUNT; i++) s_errors[i] = 0;
    return ESP_OK;
}

esp_err_t sensor_ctl_read(sensor_ctl_type_t type, float *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    int ret = -1;
    switch (type) {
        case SENSOR_TYPE_TEMPERATURE:
            ret = ds18b20_read_temperature(out);
            break;
        case SENSOR_TYPE_CURRENT:
            ret = acs712_read_current_a(out);
            break;
        case SENSOR_TYPE_POWER:
            ret = pzem_read_power_w(out);
            break;
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
    if (ret != 0) {
        s_errors[type]++;
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t sensor_ctl_read_all(sensor_ctl_snapshot_t *snap)
{
    if (!snap) return ESP_ERR_INVALID_ARG;
    esp_err_t err;
    err = sensor_ctl_read(SENSOR_TYPE_TEMPERATURE, &snap->temperature_c);
    if (err != ESP_OK) snap->temperature_c = -999.0f;
    err = sensor_ctl_read(SENSOR_TYPE_CURRENT, &snap->current_a);
    if (err != ESP_OK) snap->current_a = -1.0f;
    err = sensor_ctl_read(SENSOR_TYPE_POWER, &snap->power_w);
    if (err != ESP_OK) snap->power_w = -1.0f;
    return ESP_OK;
}

int sensor_ctl_get_last_error_count(sensor_ctl_type_t type)
{
    if ((int)type < 0 || type >= SENSOR_TYPE_COUNT) return 0;
    return s_errors[type];
}

void sensor_ctl_reset_error_count(sensor_ctl_type_t type)
{
    if ((int)type < 0 || type >= SENSOR_TYPE_COUNT) return;
    s_errors[type] = 0;
}
