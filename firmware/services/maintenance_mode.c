// @requirement RF-UI-MENU-003 / RF-UI-MENU-003.1 Modo de manutenção configurável
#include "maintenance_mode.h"
#include "config_manager.h"
#include "global_state.h"
#include "audit_log.h"
#include "alert_manager.h"
#include "alm_ids.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "maint_mode";
static uint64_t s_expire_at_s = 0;
static bool s_pre_expiry_notified = false;

#define MAINT_PRE_EXPIRY_NOTIFY_S 300U

extern global_state_t g_gs;

esp_err_t maintenance_mode_init(void)
{
    const system_params_storage_t *sys = config_get_system();
    g_gs.maintenance_mode = sys ? sys->maintenance_mode : false;
    s_expire_at_s = 0;
    ESP_LOGI(TAG, "init maintenance=%d", (int)g_gs.maintenance_mode);
    return ESP_OK;
}

bool maintenance_mode_is_active(void)
{
    return g_gs.maintenance_mode;
}

esp_err_t maintenance_mode_activate(uint32_t duration_s)
{
    system_params_storage_t sys = *config_get_system();
    sys.maintenance_mode = true;
    config_set_system(&sys);
    g_gs.maintenance_mode = true;
    uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
    s_expire_at_s = (duration_s > 0) ? (now_s + duration_s) : 0;
    s_pre_expiry_notified = false;
    audit_log_event(AUDIT_MAINTENANCE, "Maintenance mode activated");
    ESP_LOGW(TAG, "Manutencao ATIVA (%lus)", (unsigned long)duration_s);
    return ESP_OK;
}

esp_err_t maintenance_mode_deactivate(void)
{
    system_params_storage_t sys = *config_get_system();
    sys.maintenance_mode = false;
    config_set_system(&sys);
    g_gs.maintenance_mode = false;
    s_expire_at_s = 0;
    s_pre_expiry_notified = false;
    alert_manager_clear(ALM_064);
    audit_log_event(AUDIT_MAINTENANCE, "Maintenance mode deactivated");
    ESP_LOGI(TAG, "Manutencao DESATIVADA");
    return ESP_OK;
}

void maintenance_mode_tick(uint64_t now_s)
{
    if (!g_gs.maintenance_mode || s_expire_at_s == 0) return;
    uint32_t rem = maintenance_mode_remaining_s(now_s);
    if (rem > 0 && rem <= MAINT_PRE_EXPIRY_NOTIFY_S && !s_pre_expiry_notified) {
        s_pre_expiry_notified = true;
        audit_log_event(AUDIT_MAINTENANCE, "Maintenance expiring soon");
        if (alert_manager_is_active(ALM_064)) {
            alert_manager_raise_full(ALM_064, ALERT_SEVERITY_WARNING, ALERT_CATEGORY_SYSTEM,
                                     "Manutencao expira em breve", 0.0f,
                                     "Finalizar manutencao ou renovar", 0, false, false, now_s);
        }
        ESP_LOGW(TAG, "Manutencao expira em %lus", (unsigned long)rem);
    }
    if (now_s >= s_expire_at_s) {
        maintenance_mode_deactivate();
    }
}

uint32_t maintenance_mode_remaining_s(uint64_t now_s)
{
    if (!g_gs.maintenance_mode || s_expire_at_s == 0 || now_s >= s_expire_at_s) return 0;
    return (uint32_t)(s_expire_at_s - now_s);
}

bool maintenance_mode_is_expiring_soon(uint64_t now_s)
{
    if (!g_gs.maintenance_mode || s_expire_at_s == 0) return false;
    uint32_t rem = maintenance_mode_remaining_s(now_s);
    return (rem > 0 && rem <= MAINT_PRE_EXPIRY_NOTIFY_S);
}
