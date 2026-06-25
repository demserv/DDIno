#ifndef DRIVER_ACS712_H
#define DRIVER_ACS712_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define ACS712_CHANNEL_COUNT  (10U)
#define ACS712_ADC_REF_MV     (3300.0f)
#define ACS712_MV_PER_A       (100.0f)
#define ACS712_ZERO_OFFSET_MV (1650.0f)

typedef struct {
    int      cs_gpio;
    uint8_t  adc_channel;
    float    zero_offset_mv;
    float    mv_per_a;
    float    current_a;
    bool     valid;
} acs712_chan_t;

esp_err_t acs712_init(void);
esp_err_t acs712_read_plug(uint8_t plug_id, float *current_a);
esp_err_t acs712_calibrate_zero(uint8_t plug_id);
void      acs712_set_zero_offset(uint8_t plug_id, float offset_mv);
float     acs712_get_zero_offset(uint8_t plug_id);

#endif
