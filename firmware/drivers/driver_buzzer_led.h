// @requirement RF-LED-001 a RF-LED-003 Sinalização LED RGB + buzzer
#ifndef FIRMWARE_DRIVERS_BUZZER_LED_H
#define FIRMWARE_DRIVERS_BUZZER_LED_H

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    LED_OFF = 0,
    LED_GREEN,
    LED_YELLOW,
    LED_RED
} led_color_t;

esp_err_t buzzer_led_init(void);
void buzzer_beep(uint32_t duration_ms);
void led_set(led_color_t color);
void led_all_on(void);
void led_all_off(void);
void buzzer_led_alert(void);
void buzzer_led_clear(void);
void buzzer_set_mute(uint32_t duration_ms);
void buzzer_clear_mute(void);
bool buzzer_is_muted(void);

#endif
