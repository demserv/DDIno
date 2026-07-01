// @requirement RF-ALERT-001/RF-ALERT-003 Tabela can├┤nica de metadados de ALMs
// (severidade, categoria, ACK, auto-clear, sugest├úo de a├º├úo) ÔÇö Tabela 9.5-A da SRS v3.11.
#ifndef FIRMWARE_INCLUDE_ALM_CATALOG_H
#define FIRMWARE_INCLUDE_ALM_CATALOG_H

#include <stdbool.h>
#include <stdint.h>
#include "alert_model.h"

typedef struct {
    alert_severity_t severity;
    alert_category_t category;
    bool             ack_req;
    bool             auto_clear;
    const char      *action_hint; /* NULL quando "ÔÇö" na tabela can├┤nica */
} alm_meta_t;

/* Retorna os metadados can├┤nicos do ALM (1..65) ou NULL se fora de faixa. */
const alm_meta_t *alm_catalog_get(int16_t alm_id);

#endif
