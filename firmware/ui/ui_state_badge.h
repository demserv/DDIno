// @requirement RF-GLOBAL-003 Badge de estado global na UI
#ifndef FIRMWARE_UI_STATE_BADGE_H
#define FIRMWARE_UI_STATE_BADGE_H

#include "esp_err.h"

esp_err_t ui_state_badge_init(void);
void ui_state_badge_update(void);

#endif
