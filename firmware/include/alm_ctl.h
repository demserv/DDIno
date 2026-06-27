// @requirement RF-ALERT-007 a RF-ALERT-020 Gerenciamento estendido de alertas: escalonamento, dedup, som
#ifndef FIRMWARE_INCLUDE_ALM_CTL_H
#define FIRMWARE_INCLUDE_ALM_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ALM_SEVERITY_LOW = 0,
    ALM_SEVERITY_MEDIUM,
    ALM_SEVERITY_HIGH,
    ALM_SEVERITY_CRITICAL
} alm_ctl_severity_t;

typedef struct {
    uint32_t id;
    alm_ctl_severity_t severity;
    char code[16];
    char message[128];
    uint32_t timestamp_s;
    bool acked;
} alm_ctl_entry_t;

esp_err_t alm_ctl_init(void);
esp_err_t alm_ctl_raise(const char *code, alm_ctl_severity_t severity, const char *msg);
esp_err_t alm_ctl_ack(uint32_t id);
esp_err_t alm_ctl_silence(uint32_t timeout_s);
esp_err_t alm_ctl_unsilence(void);
bool alm_ctl_is_silenced(void);
int alm_ctl_get_active_count(void);
alm_ctl_entry_t *alm_ctl_get_active(int index);
esp_err_t alm_ctl_dedup_cooldown(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
