// @requirement RF-UI-CAROUSEL-001 Gerenciamento de telas com content_root
// @requirement RF-UI-STATUS-001 Topbar e footer persistentes
#include "ui_screen_manager.h"
#include "ui_theme.h"

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

static lv_obj_t *g_content_root = NULL;
static ui_root_vm_t *g_vm = NULL;
static ui_screen_id_t g_current_screen = UI_SCREEN_DASHBOARD;

typedef void (*ui_screen_create_fn_t)(lv_obj_t *, ui_root_vm_t *);
typedef void (*ui_screen_update_fn_t)(ui_root_vm_t *);

static ui_screen_create_fn_t g_create_fns[12];
static ui_screen_update_fn_t g_update_fns[12];

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

    ui_screen_manager_show(UI_SCREEN_DASHBOARD);
}

void ui_screen_manager_show(ui_screen_id_t screen_id)
{
    if (screen_id >= 12) return;
    if (!g_content_root) return;
    if (!g_create_fns[screen_id]) return;

    lv_obj_clean(g_content_root);
    g_current_screen = screen_id;

    g_create_fns[screen_id](g_content_root, g_vm);

    if (g_vm) {
        g_vm->footer.page_index = (uint8_t)screen_id;
    }
}

void ui_screen_manager_refresh(void)
{
    if (!g_vm) return;
    if (!g_update_fns[g_current_screen]) return;
    g_update_fns[g_current_screen](g_vm);
}

ui_screen_id_t ui_screen_manager_get_current(void)
{
    return g_current_screen;
}
