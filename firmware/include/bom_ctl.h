// @requirement BOM Audit — verificação de hardware presente vs. esperado
#ifndef FIRMWARE_INCLUDE_BOM_CTL_H
#define FIRMWARE_INCLUDE_BOM_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOM_STATUS_PRESENT = 0,
    BOM_STATUS_MISSING,
    BOM_STATUS_FAULTY,
    BOM_STATUS_UNKNOWN
} bom_ctl_status_t;

typedef struct {
    const char *name;
    bom_ctl_status_t status;
    bool critical;
} bom_ctl_item_t;

esp_err_t bom_ctl_init(void);
int bom_ctl_get_item_count(void);
const bom_ctl_item_t *bom_ctl_get_item(int index);
esp_err_t bom_ctl_scan(void);
bool bom_ctl_all_present(void);
bool bom_ctl_any_critical_missing(void);

#ifdef __cplusplus
}
#endif

#endif
