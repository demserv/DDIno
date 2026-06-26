#include "ui_app.h"
#include "ui_theme.h"
#include "ui_view_model.h"
#include "components/ui_topbar.h"
#include "components/ui_footer.h"
#include "components/ui_critical_overlay.h"
#include "ui_screen_manager.h"

#include "lvgl.h"

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

    ui_screen_manager_init(root, &g_vm);

    ui_footer_update(&g_footer, &g_vm.footer);
    ui_topbar_update(&g_topbar, &g_vm.topbar);
}

void ui_app_tick(void)
{
    /* TODO: Integrar com servicos reais quando disponiveis */
    /* ui_view_model_update_from_system(&g_vm); */

    ui_topbar_update(&g_topbar, &g_vm.topbar);
    ui_footer_update(&g_footer, &g_vm.footer);
    ui_screen_manager_refresh();

    /* Overlays criticos baseados no estado do sistema */
    if (g_vm.footer.system_state == UI_SYSTEM_EMERGENCY) {
        ui_critical_overlay_show(&g_emergency_overlay, true);
        ui_critical_overlay_show(&g_safeoff_overlay, false);
    } else if (g_vm.footer.system_state == UI_SYSTEM_SAFE_OFF) {
        ui_critical_overlay_show(&g_safeoff_overlay, true);
        ui_critical_overlay_show(&g_emergency_overlay, false);
    } else {
        ui_critical_overlay_show(&g_safeoff_overlay, false);
        ui_critical_overlay_show(&g_emergency_overlay, false);
    }
}
