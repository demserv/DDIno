// @requirement RF-WEB-006 Recuperação local de senha web via arquivo no SD
#include "auth_recovery_sd.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "services/audit_log.h"
#include "storage_sd.h"
#include "web/api_auth.h"

static const char *TAG = "auth_recovery";
#define RECOVERY_PATH "/sdcard/config/recovery/auth_reset.token"

esp_err_t auth_recovery_sd_init(void)
{
    return ESP_OK;
}

esp_err_t auth_recovery_sd_process(bool maintenance_mode)
{
    if (!maintenance_mode) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!storage_sd_is_mounted()) {
        ESP_LOGW(TAG, "SD indisponivel para recovery");
        return ESP_ERR_INVALID_STATE;
    }

    FILE *f = fopen(RECOVERY_PATH, "r");
    if (!f) {
        return ESP_OK;
    }

    char token[64];
    if (!fgets(token, sizeof(token), f)) {
        fclose(f);
        return ESP_ERR_INVALID_RESPONSE;
    }
    fclose(f);

    size_t len = strlen(token);
    while (len > 0 && (token[len - 1] == '\n' || token[len - 1] == '\r')) {
        token[--len] = '\0';
    }
    if (len < 8 || strcmp(token, "DDINO_RESET_AUTH") != 0) {
        ESP_LOGW(TAG, "Token recovery invalido");
        audit_log_event(AUDIT_MAINTENANCE, "auth recovery SD rejected: invalid token");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = api_auth_reset_password_to_default();
    if (err != ESP_OK) {
        audit_log_event(AUDIT_MAINTENANCE, "auth recovery SD failed");
        return err;
    }

    remove(RECOVERY_PATH);
    audit_log_event(AUDIT_MAINTENANCE, "auth recovery SD: password reset executed");
    ESP_LOGW(TAG, "Senha admin resetada via token SD");
    return ESP_OK;
}
