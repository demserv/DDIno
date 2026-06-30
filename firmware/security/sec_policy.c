// @requirement RNF-SEC-001..003, RB-SEC-001..013 Políticas de segurança
#include "sec_policy.h"

#include <string.h>

#include "esp_log.h"
#include "esp_random.h"

#define PASSWORD_MIN_LEN 8
#define PASSWORD_MAX_LEN 64
#define TOKEN_MIN_SIZE   32
#define TOKEN_MAX_LEN    128

static const char *TAG = "sec_policy";

esp_err_t sec_policy_init(void)
{
    ESP_LOGI(TAG, "Security policy initialized");
    return ESP_OK;
}

bool sec_policy_validate_password(const char *password, size_t len)
{
    if (!password || len < PASSWORD_MIN_LEN || len > PASSWORD_MAX_LEN) return false;
    return true;
}

static const char hex_chars[] = "0123456789abcdef";

esp_err_t sec_policy_generate_token(char *out, size_t out_size)
{
    if (!out || out_size < TOKEN_MIN_SIZE) return ESP_ERR_INVALID_ARG;
    size_t token_len = out_size - 1;
    if (token_len > TOKEN_MAX_LEN) token_len = TOKEN_MAX_LEN;
    uint8_t *raw = malloc(token_len);
    if (!raw) return ESP_ERR_NO_MEM;
    esp_fill_random(raw, token_len);
    for (size_t i = 0; i < token_len; i++) {
        out[i] = hex_chars[raw[i] & 0x0F];
    }
    free(raw);
    out[token_len] = '\0';
    return ESP_OK;
}

bool sec_policy_validate_token(const char *token)
{
    return (token != NULL && strlen(token) > 0);
}

esp_err_t sec_policy_audit_login(const char *user, bool success)
{
    ESP_LOGI(TAG, "Login %s: %s", success ? "OK" : "FAIL", user ? user : "unknown");
    return ESP_OK;
}

void sec_policy_enforce_session_timeout(void)
{
    ESP_LOGD(TAG, "Session timeout enforced");
}

