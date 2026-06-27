// @requirement RF-STORAGE-001..010, RB-STOR-001..017, RF-PERSIST-001, RF-PERSIST-ATOMIC-001, RF-PERSIST-EXPORT-001
#ifndef FIRMWARE_INCLUDE_STORAGE_MANAGER_H
#define FIRMWARE_INCLUDE_STORAGE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STORAGE_NS_CFG = 0,
    STORAGE_NS_CAL,
    STORAGE_NS_PROFILE,
    STORAGE_NS_STATE,
    STORAGE_NS_LOG,
    STORAGE_NS_WIZARD,
    STORAGE_NS_TIME,
    STORAGE_NS_ALM,
    STORAGE_NS_COUNT
} storage_namespace_t;

#define STORAGE_SCHEMA_CFG_VERSION     1
#define STORAGE_SCHEMA_CAL_VERSION     1
#define STORAGE_SCHEMA_PROFILE_VERSION 1
#define STORAGE_SCHEMA_STATE_VERSION   1
#define STORAGE_SCHEMA_LOG_VERSION     1
#define STORAGE_SCHEMA_WIZARD_VERSION  1
#define STORAGE_SCHEMA_TIME_VERSION    1
#define STORAGE_SCHEMA_ALM_VERSION     1

typedef struct {
    uint32_t schema_version;
    uint32_t crc32;
    uint64_t timestamp_us;
} storage_metadata_t;

esp_err_t storage_manager_init(void);
esp_err_t storage_manager_deinit(void);
esp_err_t storage_open(storage_namespace_t ns, bool read_only);
esp_err_t storage_commit(storage_namespace_t ns);
esp_err_t storage_close(storage_namespace_t ns);

esp_err_t storage_set_u32(storage_namespace_t ns, const char *key, uint32_t value);
esp_err_t storage_get_u32(storage_namespace_t ns, const char *key, uint32_t *out, uint32_t default_value);
esp_err_t storage_set_i32(storage_namespace_t ns, const char *key, int32_t value);
esp_err_t storage_get_i32(storage_namespace_t ns, const char *key, int32_t *out, int32_t default_value);
esp_err_t storage_set_str(storage_namespace_t ns, const char *key, const char *value);
esp_err_t storage_get_str(storage_namespace_t ns, const char *key, char *out, size_t out_size, const char *default_value);
esp_err_t storage_set_blob(storage_namespace_t ns, const char *key, const void *buf, size_t len);
esp_err_t storage_get_blob(storage_namespace_t ns, const char *key, void *buf, size_t *len);

uint32_t storage_get_schema_version(storage_namespace_t ns);
esp_err_t storage_migrate_if_needed(storage_namespace_t ns);

esp_err_t storage_export_to_buffer(storage_namespace_t ns, uint8_t *buf, size_t *len);
esp_err_t storage_import_from_buffer(storage_namespace_t ns, const uint8_t *buf, size_t len);
esp_err_t storage_erase_namespace(storage_namespace_t ns);

#ifdef __cplusplus
}
#endif

#endif
