// @requirement RF-GLOBAL-003 Entrada por keypad como fallback local
// @requirement RNF-HARDWARE-001 AD Keypad 5V com condicionamento via MCP3208 CH3
#ifndef FIRMWARE_DRIVERS_DRIVER_AD_KEYPAD_H
#define FIRMWARE_DRIVERS_DRIVER_AD_KEYPAD_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_ESC,
    KEY_MENU,
    KEY_FEED
} ad_keypad_key_t;

typedef void (*ad_keypad_callback_t)(ad_keypad_key_t key);

esp_err_t ad_keypad_init(ad_keypad_callback_t callback);
void ad_keypad_set_callback(ad_keypad_callback_t callback);
ad_keypad_key_t ad_keypad_read(void);

#endif
