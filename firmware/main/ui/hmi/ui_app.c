// @requirement RF-UI-CAROUSEL-001 Gerenciamento de telas e overlays
// @requirement RF-UI-OVERLAY-001 Overlays críticos SAFE_OFF/EMERGENCY
// @requirement RF-UI-WIZARD-001..005 Integração com wizard
#include "ui_app.h"
#include "ui_theme.h"
#include "ui_view_model.h"
#include "ui_events.h"
#include "components/ui_topbar.h"
#include "components/ui_footer.h"
#include "components/ui_critical_overlay.h"
#include "components/ui_inline_hint.h"
#include "ui_screen_manager.h"
#include "driver_ad_keypad_lvgl.h"
#include "config_manager.h"

#include "lvgl.h"

static ui_inline_hint_t g_hint;

/* @requirement RF-UI-NAV-HOME-001 / RF-UI-SHORTCUT-001 / RF-UI-MUTE-001 Teclas especiais. */
static void keypad_special_handler(int key)
{
    if (key == LV_KEY_HOME) {
        ui_screen_manager_go_home();
    } else if (key == LV_KEY_FEED) {
        ui_topbar_show_feed_confirm();
    } else if (key == LV_KEY_MUTE) {
        ui_events_emit(UI_EVENT_REQUEST_MUTE);
        ui_inline_hint_show(&g_hint, LV_SYMBOL_MUTE " MUTE ativo — alertas silenciados", 3000U);
    }
}

static ui_root_vm_t g_vm;
static ui_topbar_t g_topbar;
static ui_footer_t g_footer;
static ui_critical_overlay_t g_safeoff_overlay;
static ui_critical_overlay_t g_emergency_overlay;

void ui_app_init(void)
{
    ui_theme_init();
    ui_view_model_init_defaults(&g_vm);

    lv_obj_t *root = lv_scr_act();
    lv_obj_set_style_bg_color(root, UI_COLOR_BG, 0);

    ui_topbar_create(&g_topbar, root);
    ui_footer_create(&g_footer, root);

    ui_critical_overlay_safeoff_create(&g_safeoff_overlay, lv_layer_top());
    ui_critical_overlay_emergency_create(&g_emergency_overlay, lv_layer_top());

    ui_inline_hint_create(&g_hint, root);

    ui_screen_manager_init(root, &g_vm);

    driver_ad_keypad_set_special_cb(keypad_special_handler);

    ui_events_set_mute_duration((ui_mute_duration_t)config_get_mute_duration());

    ui_footer_update(&g_footer, &g_vm.footer);
    ui_topbar_update(&g_topbar, &g_vm.topbar);
}

void ui_app_refresh_now(void)
{
    ui_view_model_update_from_system(&g_vm);
    ui_topbar_update(&g_topbar, &g_vm.topbar);
    ui_footer_update(&g_footer, &g_vm.footer);
    ui_screen_manager_refresh();
}

void ui_app_tick(void)
{
    driver_ad_keypad_gesture_poll();
    ui_view_model_update_from_system(&g_vm);

    ui_topbar_update(&g_topbar, &g_vm.topbar);
    ui_footer_update(&g_footer, &g_vm.footer);
    ui_screen_manager_refresh();
    ui_screen_manager_tick();

    /* Overlays criticos baseados no estado do sistema */
    if (g_vm.footer.system_state == UI_SYSTEM_EMERGENCY) {
        ui_critical_overlay_update_cause(&g_emergency_overlay, &g_vm);
        ui_critical_overlay_show(&g_emergency_overlay, true);
        ui_critical_overlay_show(&g_safeoff_overlay, false);
    } else if (g_vm.footer.system_state == UI_SYSTEM_SAFE_OFF) {
        ui_critical_overlay_update_cause(&g_safeoff_overlay, &g_vm);
        ui_critical_overlay_show(&g_safeoff_overlay, true);
        ui_critical_overlay_show(&g_emergency_overlay, false);
    } else {
        ui_critical_overlay_show(&g_safeoff_overlay, false);
        ui_critical_overlay_show(&g_emergency_overlay, false);
    }
}
