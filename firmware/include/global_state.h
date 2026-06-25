// @requirement RF-UI-WIZARD-001..005 Wizard com steps individuais
#ifndef FIRMWARE_INCLUDE_GLOBAL_STATE_H
#define FIRMWARE_INCLUDE_GLOBAL_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "system_types.h"
#include "fsm/feed_fsm.h"

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
} global_state_t;

#endif
