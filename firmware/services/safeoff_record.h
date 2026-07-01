#ifndef SERVICES_SAFEOFF_RECORD_H
#define SERVICES_SAFEOFF_RECORD_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "system_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAFEOFF_RECORD_MAX_ENTRIES 16

typedef struct {
    safeoff_reason_t reason;
    char             source_alm[16];
    char             entered_at[32];
    uint64_t         entered_epoch_s;
    uint64_t         duration_s;
    bool             resolved;
    char             resolved_at[32];
} safeoff_record_entry_t;

typedef struct {
    uint32_t              count;
    safeoff_record_entry_t entries[SAFEOFF_RECORD_MAX_ENTRIES];
} safeoff_record_t;

esp_err_t safeoff_record_init(void);
esp_err_t safeoff_record_append(safeoff_reason_t reason, const char *source_alm, uint64_t now_s);
esp_err_t safeoff_record_resolve_latest(uint64_t now_s);
const safeoff_record_t *safeoff_record_get(void);
esp_err_t safeoff_record_persist(void);
esp_err_t safeoff_record_load(void);

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /* SERVICES_SAFEOFF_RECORD_H */