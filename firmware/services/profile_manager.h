// @requirement RF-UI-MENU-002 / RF-PERSIST-PROFILE-001 Perfis de configuração
#ifndef FIRMWARE_SERVICES_PROFILE_MANAGER_H
#define FIRMWARE_SERVICES_PROFILE_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PROFILE_MANAGER_MAX 8
#define PROFILE_NAME_MAX    24

esp_err_t profile_manager_init(void);
esp_err_t profile_manager_list(char names[][PROFILE_NAME_MAX], uint8_t *count);
esp_err_t profile_manager_save(const char *name);
esp_err_t profile_manager_load(const char *name);
esp_err_t profile_manager_rename(const char *old_name, const char *new_name);
esp_err_t profile_manager_delete(const char *name);
bool      profile_manager_is_name_valid(const char *name);

#endif
