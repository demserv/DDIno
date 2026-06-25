#ifndef FIRMWARE_WEB_API_AUTH_H
#define FIRMWARE_WEB_API_AUTH_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define API_AUTH_MAX_TOKENS 8
#define API_TOKEN_STR_LEN   65
#define API_USER_MAX        32
#define API_PASS_MAX        64

typedef struct {
    char token[API_TOKEN_STR_LEN];
    char user[API_USER_MAX];
    uint64_t expires_ms;
    uint64_t created_ms;
} api_auth_token_t;

esp_err_t api_auth_init(void);
bool api_auth_validate(const char *token);
const char* api_auth_login(const char *user, const char *password);
esp_err_t api_auth_logout(const char *token);
int api_auth_active_count(void);

#endif
