// @requirement RF-GLOBAL-001 Definição de estados globais
// @requirement RF-GLOBAL-002 Transições de estado global com rastreabilidade
#ifndef FIRMWARE_INCLUDE_GLOBAL_STATE_H
#define FIRMWARE_INCLUDE_GLOBAL_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "system_types.h"
#include "alm_ids.h"
#include "fsm/feed_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIZARD_STEP_NONE = 0,
    WIZARD_STEP_WELCOME,
    WIZARD_STEP_PASSWORD,
    WIZARD_STEP_THERMAL,
    WIZARD_STEP_ATO,
    WIZARD_STEP_ELECTRIC,
    WIZARD_STEP_REVIEW,
    WIZARD_STEP_COMPLETE
} wizard_step_t;

typedef enum {
    GS_HEALTH_TEMP = 0,
    GS_HEALTH_ATO,
    GS_HEALTH_PZEM,
    GS_HEALTH_SD,
    GS_HEALTH_WIFI,
    GS_HEALTH_UI,
    GS_HEALTH_ELECTRIC,
    GS_HEALTH_SELFTEST
} global_state_health_field_t;

typedef struct {
    system_state_t       system_state;
    bool                 maintenance_mode;
    bool                 electric_ok;
    bool                 selftest_passed;
    bool                 temp_ok;
    bool                 ato_ok;
    bool                 pzem_ok;
    bool                 sd_ok;
    bool                 wifi_ok;
    bool                 ui_ok;
    bool                 hw_ok;
    bool                 wizard_completed;
    bool                 observation_mode;
    bool                 monitor_only_mode;
    alm_category_mask_t  active_alm_categories;
    uint16_t             active_alerts_count;
    uint16_t             critical_alerts_count;
    reset_reason_t       last_reset_reason;
    uint64_t             uptime_s;
    char                 config_schema_version[16];
    char                 fw_version[32];
    char                 srs_version[32];
    uint64_t             last_health_check_timestamp;
    bool                 time_valid;
    time_source_t        time_source;
    safeoff_reason_t     safeoff_reason;
    char                 safeoff_entered_at[32];
    char                 safeoff_source_alm[16];
    bool                 feed_active;
    uint32_t             feed_remaining_s;
    feed_state_t         feed_state;
    uint64_t             health_check_interval_s;
    float                temp_filtered_c;
    bool                 restart_in_progress;
    wizard_step_t        wizard_step;
    bool                 hw_alert_pending;
    alm_id_t             hw_alert_alm_id;
    char                 hw_alert_msg[128];
    char                 reset_status_msg[64];
} global_state_t;

esp_err_t global_state_init(void);
void global_state_bind(global_state_t *gs);
esp_err_t global_state_get_snapshot(global_state_t *out);
const global_state_t *global_state_get_snapshot_ptr(void);
esp_err_t global_state_transition(system_state_t next_state, safeoff_reason_t reason,
                                   const char *source_alm, const char *source_module,
                                   uint64_t now_s);
esp_err_t global_state_set_health_flag(global_state_health_field_t field, bool ok);
global_state_t *global_state_get_write_ptr(void);

/* @requirement RNF-GLOBAL-ANTIFLAP-001 Anti-flap compartilhado (NVS via config_get_antiflap). */
bool global_state_antiflap_allow(uint64_t now_ms);
void global_state_antiflap_commit(uint64_t now_ms);

/* @requirement RF-WEB-008 Sincroniza RAM runtime após import/persistência de system/calib. */
void global_state_sync_from_config(global_state_t *gs);

#ifdef __cplusplus
}
#endif

#endif
