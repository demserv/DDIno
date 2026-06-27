// @requirement RF-DISP-001 a RF-DISP-035 Gerenciamento de display: brilho, contraste, sleep, orientação
#ifndef FIRMWARE_INCLUDE_DISP_CTL_H
#define FIRMWARE_INCLUDE_DISP_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t brightness_percent;
    uint8_t contrast;
    bool backlight_on;
    bool sleep_mode;
} disp_ctl_state_t;

const char *disp_ctl_get_name(void);
esp_err_t disp_ctl_init(void);
esp_err_t disp_ctl_set_brightness(uint8_t percent);
uint8_t disp_ctl_get_brightness(void);
esp_err_t disp_ctl_set_contrast(uint8_t contrast);
uint8_t disp_ctl_get_contrast(void);
esp_err_t disp_ctl_backlight_on(void);
esp_err_t disp_ctl_backlight_off(void);
esp_err_t disp_ctl_sleep(void);
esp_err_t disp_ctl_wake(void);
bool disp_ctl_is_sleeping(void);
esp_err_t disp_ctl_get_state(disp_ctl_state_t *state);
esp_err_t disp_ctl_set_rotation(int degrees);

#ifdef __cplusplus
}
#endif

#endif
