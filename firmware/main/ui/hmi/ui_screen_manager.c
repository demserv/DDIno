// @requirement RF-UI-CAROUSEL-001 Gerenciamento de telas com content_root e carrossel
// @requirement RF-UI-STATUS-001 Topbar e footer persistentes
#include "ui_screen_manager.h"
#include "ui_theme.h"
#include "hardware_config.h"
#include "global_state.h"

#include "screens/ui_screen_dashboard.h"
#include "screens/ui_screen_devices_page1.h"
#include "screens/ui_screen_devices_page2.h"
#include "screens/ui_screen_energy.h"
#include "screens/ui_screen_main_menu.h"
#include "screens/ui_screen_config_temperature.h"
#include "screens/ui_screen_alerts.h"
#include "screens/ui_screen_feed_active.h"
#include "screens/ui_screen_diagnostics.h"
#include "screens/ui_screen_system.h"
#include "screens/ui_screen_logs.h"
#include "screens/ui_screen_wizard.h"
#include "screens/ui_screen_ato.h"
#include "screens/ui_screen_diag_detail.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ui_scr_mgr";

static lv_obj_t *g_content_root = NULL;
static ui_root_vm_t *g_vm = NULL;
static ui_screen_id_t g_current_screen = UI_SCREEN_DASHBOARD;

static bool s_carousel_enabled = true;
static bool s_carousel_paused = false;
static uint64_t s_last_carousel_ms = 0;
static uint64_t s_last_interaction_ms = 0;

/* @requirement RF-UI-CAROUSEL-001 Ordem canônica do carrossel incluindo ATO e Alarmes. */
static const ui_screen_id_t s_carousel_screens[] = {
    UI_SCREEN_DASHBOARD,
    UI_SCREEN_DEVICES_1,
    UI_SCREEN_DEVICES_2,
    UI_SCREEN_ENERGY,
    UI_SCREEN_ATO,
    UI_SCREEN_ALERTS,
    UI_SCREEN_DIAGNOSTICS
};
#define CAROUSEL_SCREEN_COUNT (sizeof(s_carousel_screens) / sizeof(s_carousel_screens[0]))

typedef void (*ui_screen_create_fn_t)(lv_obj_t *, ui_root_vm_t *);
typedef void (*ui_screen_update_fn_t)(ui_root_vm_t *);

static ui_screen_create_fn_t g_create_fns[14];
static ui_screen_update_fn_t g_update_fns[14];

extern global_state_t g_gs;

static void register_screens(void)
{
    g_create_fns[UI_SCREEN_DASHBOARD] = ui_screen_dashboard_create;
    g_update_fns[UI_SCREEN_DASHBOARD] = ui_screen_dashboard_update;

    g_create_fns[UI_SCREEN_DEVICES_1] = ui_screen_devices_page1_create;
    g_update_fns[UI_SCREEN_DEVICES_1] = ui_screen_devices_page1_update;

    g_create_fns[UI_SCREEN_DEVICES_2] = ui_screen_devices_page2_create;
    g_update_fns[UI_SCREEN_DEVICES_2] = ui_screen_devices_page2_update;

    g_create_fns[UI_SCREEN_ENERGY] = ui_screen_energy_create;
    g_update_fns[UI_SCREEN_ENERGY] = ui_screen_energy_update;

    g_create_fns[UI_SCREEN_MAIN_MENU] = ui_screen_main_menu_create;
    g_update_fns[UI_SCREEN_MAIN_MENU] = ui_screen_main_menu_update;

    g_create_fns[UI_SCREEN_CONFIG_TEMPERATURE] = ui_screen_config_temperature_create;
    g_update_fns[UI_SCREEN_CONFIG_TEMPERATURE] = ui_screen_config_temperature_update;

    g_create_fns[UI_SCREEN_ALERTS] = ui_screen_alerts_create;
    g_update_fns[UI_SCREEN_ALERTS] = ui_screen_alerts_update;

    g_create_fns[UI_SCREEN_FEED_ACTIVE] = ui_screen_feed_active_create;
    g_update_fns[UI_SCREEN_FEED_ACTIVE] = ui_screen_feed_active_update;

    g_create_fns[UI_SCREEN_DIAGNOSTICS] = ui_screen_diagnostics_create;
    g_update_fns[UI_SCREEN_DIAGNOSTICS] = ui_screen_diagnostics_update;

    g_create_fns[UI_SCREEN_SYSTEM] = ui_screen_system_create;
    g_update_fns[UI_SCREEN_SYSTEM] = ui_screen_system_update;

    g_create_fns[UI_SCREEN_LOGS] = ui_screen_logs_create;
    g_update_fns[UI_SCREEN_LOGS] = ui_screen_logs_update;

    g_create_fns[UI_SCREEN_WIZARD] = ui_screen_wizard_create;
    g_update_fns[UI_SCREEN_WIZARD] = ui_screen_wizard_update;

    g_create_fns[UI_SCREEN_ATO] = ui_screen_ato_create;
    g_update_fns[UI_SCREEN_ATO] = ui_screen_ato_update;

    g_create_fns[UI_SCREEN_DIAG_DETAIL] = ui_screen_diag_detail_create;
    g_update_fns[UI_SCREEN_DIAG_DETAIL] = ui_screen_diag_detail_update;
}

/* @requirement RF-UI-INPUT-001/002 Navegação por keypad sem touch: após montar a
 * tela, repovoa o grupo de foco padrão com todos os objetos clicáveis (botões,
 * tiles, linhas). Centralizado aqui para que TODAS as telas fiquem navegáveis sem
 * precisar de lv_group_add_obj espalhado em cada tela. */
static void focus_group_add_recursive(lv_group_t *g, lv_obj_t *obj)
{
    uint32_t n = lv_obj_get_child_cnt(obj);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) {
            lv_group_add_obj(g, child);
        }
        focus_group_add_recursive(g, child);
    }
}

static void rebuild_focus_group(void)
{
    lv_group_t *g = lv_group_get_default();
    if (!g || !g_content_root) return;
    lv_group_remove_all_objs(g);
    focus_group_add_recursive(g, g_content_root);
}

static bool carousel_blocked(void)
{
    if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF ||
        g_gs.system_state == SYSTEM_STATE_EMERGENCY) {
        return true;
    }
    if (!g_gs.wizard_completed) {
        return true;
    }
    /* @requirement RF-UI-CAROUSEL-001 Pausa o carrossel enquanto houver alerta CRÍTICO
     * ativo (antes usava hw_alert_pending, que não reflete a contagem de críticos). */
    if (g_gs.critical_alerts_count > 0) {
        return true;
    }
    if (g_gs.hw_alert_pending) {
        return true;
    }
    if (g_gs.feed_active) {
        return true;
    }
    return false;
}

/* @requirement RF-UI-CAROUSEL-001 Indica se o carrossel está pausado (para indicador
 * visual no topbar/footer). */
bool ui_screen_manager_carousel_is_paused(void)
{
    return s_carousel_paused || carousel_blocked();
}

static void carousel_advance(void)
{
    int found = -1;
    for (int i = 0; i < (int)CAROUSEL_SCREEN_COUNT; i++) {
        if (s_carousel_screens[i] == g_current_screen) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        found = 0;
    } else {
        found = (found + 1) % (int)CAROUSEL_SCREEN_COUNT;
    }
    ui_screen_manager_show(s_carousel_screens[found]);
}

void ui_screen_manager_init(lv_obj_t *root, ui_root_vm_t *vm)
{
    g_vm = vm;

    g_content_root = lv_obj_create(root);
    lv_obj_set_size(g_content_root, 480, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(g_content_root, 0, UI_CONTENT_Y);
    lv_obj_set_style_bg_color(g_content_root, UI_COLOR_BG, 0);
    lv_obj_set_style_border_width(g_content_root, 0, 0);
    lv_obj_clear_flag(g_content_root, LV_OBJ_FLAG_SCROLLABLE);

    register_screens();
    s_last_carousel_ms = esp_timer_get_time() / USEC_PER_MSEC;
    s_last_interaction_ms = s_last_carousel_ms;

    /* @requirement RF-UI-WIZARD-001 No primeiro boot (wizard não concluído) a UI abre
     * automaticamente o assistente; caso contrário, o dashboard. O carrossel já é
     * bloqueado enquanto o wizard não está concluído (carousel_blocked). */
    if (!g_gs.wizard_completed) {
        ui_screen_manager_show(UI_SCREEN_WIZARD);
    } else {
        ui_screen_manager_show(UI_SCREEN_DASHBOARD);
    }
}

void ui_screen_manager_show(ui_screen_id_t screen_id)
{
    if (screen_id >= 14) {
        ESP_LOGW(TAG, "screen_id invalido: %d", (int)screen_id);
        return;
    }
    if (!g_content_root) {
        return;
    }
    if (!g_create_fns[screen_id]) {
        ESP_LOGW(TAG, "tela %d sem factory", (int)screen_id);
        return;
    }

    lv_obj_clean(g_content_root);
    g_current_screen = screen_id;

    g_create_fns[screen_id](g_content_root, g_vm);

    /* @requirement RF-UI-INPUT-002 Torna a tela recém-montada navegável por keypad. */
    rebuild_focus_group();

    if (g_vm) {
        for (uint8_t i = 0; i < CAROUSEL_SCREEN_COUNT; i++) {
            if (s_carousel_screens[i] == screen_id) {
                g_vm->footer.page_index = i;
                g_vm->footer.page_count = (uint8_t)CAROUSEL_SCREEN_COUNT;
                break;
            }
        }
    }
}

void ui_screen_manager_refresh(void)
{
    if (!g_vm) {
        return;
    }
    if (!g_update_fns[g_current_screen]) {
        return;
    }
    g_update_fns[g_current_screen](g_vm);
}

ui_screen_id_t ui_screen_manager_get_current(void)
{
    return g_current_screen;
}

/* @requirement RF-UI-NAV-HOME-001 HOME retorna ao dashboard principal e
 * retoma o carrossel a partir da tela primária. */
void ui_screen_manager_go_home(void)
{
    ui_screen_manager_show(UI_SCREEN_DASHBOARD);
    s_carousel_paused = false;
    s_last_carousel_ms = esp_timer_get_time() / USEC_PER_MSEC;
    s_last_interaction_ms = s_last_carousel_ms;
}

void ui_screen_manager_on_user_interaction(void)
{
    s_last_interaction_ms = esp_timer_get_time() / USEC_PER_MSEC;
    s_carousel_paused = true;
}

void ui_screen_manager_carousel_pause(void)
{
    s_carousel_paused = true;
}

void ui_screen_manager_carousel_resume(void)
{
    s_carousel_paused = false;
    s_last_carousel_ms = esp_timer_get_time() / USEC_PER_MSEC;
}

void ui_screen_manager_tick(void)
{
    if (!s_carousel_enabled || s_carousel_paused || carousel_blocked()) {
        return;
    }

    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    if (now_ms - s_last_interaction_ms < HW_UI_CAROUSEL_PAUSE_ON_ACTIVITY_MS) {
        return;
    }
    if (now_ms - s_last_carousel_ms < HW_UI_CAROUSEL_INTERVAL_MS) {
        return;
    }

    bool in_carousel = false;
    for (uint8_t i = 0; i < CAROUSEL_SCREEN_COUNT; i++) {
        if (s_carousel_screens[i] == g_current_screen) {
            in_carousel = true;
            break;
        }
    }
    if (!in_carousel) {
        return;
    }

    carousel_advance();
    s_last_carousel_ms = now_ms;
}
