// @requirement RF-DISP-001 a RF-DISP-035 Gerenciamento de display
#include "disp_ctl.h"

#include "esp_log.h"

static const char *TAG = "disp_ctl";
static disp_ctl_state_t s_state = {
    .brightness_percent = 80,
    .contrast = 50,
    .backlight_on = true,
    .sleep_mode = false
};

const char *disp_ctl_get_name(void) { return "ILI9488"; }

esp_err_t disp_ctl_init(void)
{
    ESP_LOGI(TAG, "Display control initialized");
    return ESP_OK;
}

esp_err_t disp_ctl_set_brightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    s_state.brightness_percent = percent;
    ESP_LOGI(TAG, "Brightness set to %u%%", (unsigned)percent);
    return ESP_OK;
}

uint8_t disp_ctl_get_brightness(void) { return s_state.brightness_percent; }

esp_err_t disp_ctl_set_contrast(uint8_t contrast)
{
    if (contrast > 100) contrast = 100;
    s_state.contrast = contrast;
    return ESP_OK;
}

uint8_t disp_ctl_get_contrast(void) { return s_state.contrast; }

esp_err_t disp_ctl_backlight_on(void)
{
    s_state.backlight_on = true;
    s_state.sleep_mode = false;
    return ESP_OK;
}

esp_err_t disp_ctl_backlight_off(void)
{
    s_state.backlight_on = false;
    return ESP_OK;
}

esp_err_t disp_ctl_sleep(void)
{
    s_state.sleep_mode = true;
    s_state.backlight_on = false;
    return ESP_OK;
}

esp_err_t disp_ctl_wake(void)
{
    s_state.sleep_mode = false;
    s_state.backlight_on = true;
    return ESP_OK;
}

bool disp_ctl_is_sleeping(void) { return s_state.sleep_mode; }

esp_err_t disp_ctl_get_state(disp_ctl_state_t *state)
{
    if (!state) return ESP_ERR_INVALID_ARG;
    *state = s_state;
    return ESP_OK;
}

esp_err_t disp_ctl_set_rotation(int degrees)
{
    ESP_LOGI(TAG, "Rotation set to %d", degrees);
    return ESP_OK;
}

