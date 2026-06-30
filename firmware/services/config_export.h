#ifndef FIRMWARE_SERVICES_CONFIG_EXPORT_H
#define FIRMWARE_SERVICES_CONFIG_EXPORT_H

#include "esp_err.h"
#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>

esp_err_t config_export_to_json(cJSON **out_json);
esp_err_t config_import_from_json(cJSON *json, bool dry_run, bool *valid_out, uint32_t *crc_out);

#endif
