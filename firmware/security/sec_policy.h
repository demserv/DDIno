// @requirement RNF-SEC-001..003, RB-SEC-001..013 Políticas de segurança: TLS, JWT, sessões
#ifndef FIRMWARE_SECURITY_SEC_POLICY_H
#define FIRMWARE_SECURITY_SEC_POLICY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sec_policy_init(void);
bool sec_policy_validate_password(const char *password, size_t len);
esp_err_t sec_policy_generate_token(char *out, size_t out_size);
bool sec_policy_validate_token(const char *token);
esp_err_t sec_policy_audit_login(const char *user, bool success);
void sec_policy_enforce_session_timeout(void);

#ifdef __cplusplus
}
#endif

#endif
