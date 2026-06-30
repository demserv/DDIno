// @requirement RF-ALERT-004 ACK timeout enforcement
// @requirement RF-ALERT-002 Silence enforcement
#ifndef FIRMWARE_SERVICES_ALERT_MANAGER_H
#define FIRMWARE_SERVICES_ALERT_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "system_types.h"
#include "alert_model.h"

#define ALERT_SLOTS_MAX 80

typedef struct {
    int16_t           alm_id;
    bool              active;
    bool              ack_req;
    bool              acked;
    uint64_t          first_seen_ts;
    uint64_t          last_seen_ts;
    uint64_t          ack_timestamp;
    alert_severity_t  severity;
    alert_category_t  category;
    char              message[128];
    float             value;
    char              action_hint[64];
    uint16_t          related_plug_id;
    bool              state_associated;
    uint64_t          silenced_until;
} alert_slot_t;

void         alert_manager_init(void);
bool         alert_manager_raise_full(int16_t alm_id, alert_severity_t sev, alert_category_t cat,
                                      const char *msg, float val, const char *hint,
                                      uint16_t related_plug, bool ack_req, bool state_associated, uint64_t ts);
bool         alert_manager_raise(int16_t alm_id, bool ack_req, uint64_t ts);
bool         alert_manager_ack(int16_t alm_id, uint64_t ts);
bool         alert_manager_is_active(int16_t alm_id);
void         alert_manager_clear(int16_t alm_id);
void         alert_manager_check_ack_timeout(uint64_t now_s, uint32_t timeout_s);
uint16_t     alert_manager_active_count(void);
uint16_t     alert_manager_critical_count(void);
const alert_slot_t* alert_manager_get_slot(int16_t alm_id);
void         alert_manager_get_active_slots(alert_slot_t *out, uint16_t *count, uint16_t max);
void         alert_manager_get_history_slots(alert_slot_t *out, uint16_t *count, uint16_t max);
void         alert_manager_set_silenced(int16_t alm_id, uint64_t until_ts);
bool         alert_manager_is_silenced(int16_t alm_id, uint64_t now_ts);

#define ACK_STAGE_NONE     0
#define ACK_STAGE_FIRST    1
#define ACK_STAGE_CONFIRMED 2

typedef struct {
    int16_t   alm_id;
    uint8_t   ack_stage;
    uint64_t  first_ack_ts;
    bool      requires_double_ack;
} alert_ack_state_t;

bool alert_manager_ext_ack_critical(int16_t alm_id, uint64_t ts);
uint8_t alert_manager_ext_get_ack_stage(int16_t alm_id);
bool alert_manager_ext_is_critical_and_pending(int16_t alm_id);
void alert_manager_ext_reset_ack_stage(int16_t alm_id);
bool alert_manager_ext_double_ack_required(int16_t alm_id);

#endif
