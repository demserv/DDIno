// @requirement RF-UI-MENU-003 Manutenção + RF-UI-MENU-001 Reset + RF-RESET Reboot
// @requirement RF-UI-MUTE-001 MUTE via UI + RF-RESET-004 Diálogo de confirmação
#include "ui_screen_system.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include "../components/ui_menu_tile.h"
#include "../components/ui_confirm_dialog.h"
#include "../components/ui_inline_hint.h"
#include "../ui_screen_manager.h"
#include "maintenance_mode.h"
#include "global_state.h"
#include "esp_timer.h"

extern global_state_t g_gs;

static ui_menu_tile_t sys_tiles[4];
static lv_obj_t *g_status_lbl = NULL;
static ui_confirm_dialog_t g_confirm_dlg;
static ui_inline_hint_t g_hint;
static int g_pending_action = -1;
static ui_mute_duration_t g_mute_dur = UI_MUTE_DURATION_5MIN;

static void maint_toggle_cb(lv_event_t *e)
{
    (void)e;
    if (maintenance_mode_is_active()) {
        maintenance_mode_deactivate();
    } else {
        maintenance_mode_activate(3600U);
    }
}

static void ato_clear_cb(lv_event_t *e)
{
    (void)e;
    ui_events_clear_ato_blocked();
}

static void mute_btn_cb(lv_event_t *e)
{
    (void)e;
    g_mute_dur = (ui_mute_duration_t)(((int)g_mute_dur + 1) % 4);
    ui_events_set_mute_duration(g_mute_dur);
    ui_events_emit(UI_EVENT_REQUEST_MUTE);
    const char *labels[] = {"MUTE 5min", "MUTE 10min", "MUTE 15min", "MUTE ate ACK"};
    ui_inline_hint_show(&g_hint, labels[g_mute_dur], 2500);
}

static void confirm_ok_cb(lv_event_t *e)
{
    (void)e;
    ui_confirm_dialog_show(&g_confirm_dlg, false);
    switch (g_pending_action) {
        case 0:
            ui_events_emit(UI_EVENT_REQUEST_FACTORY_RESET);
            break;
        case 1:
            ui_events_emit(UI_EVENT_REQUEST_SAFE_REBOOT);
            break;
        default:
            break;
    }
    g_pending_action = -1;
}

static void confirm_cancel_cb(lv_event_t *e)
{
    (void)e;
    ui_confirm_dialog_show(&g_confirm_dlg, false);
    g_pending_action = -1;
}

static void ask_confirm(int action, const char *msg)
{
    g_pending_action = action;
    lv_label_set_text(g_confirm_dlg.message_label, msg);
    ui_confirm_dialog_show(&g_confirm_dlg, true);
}

static void sys_tile_cb(lv_event_t *e)
{
    int *action = (int *)lv_event_get_user_data(e);
    if (!action) return;
    switch (*action) {
        case 0:
            ask_confirm(0, "Reset de fabrica?\nConfirme para continuar.\n(Segunda confirmacao no tile)");
            break;
        case 1:
            ask_confirm(1, "Reiniciar o controlador agora?");
            break;
        case 2:
            ui_screen_manager_show(UI_SCREEN_LOGS);
            break;
        case 3:
            ui_screen_manager_show(UI_SCREEN_WIZARD);
            break;
    }
}

void ui_screen_system_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Manutencao");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 185, 6);

    g_status_lbl = lv_label_create(parent);
    lv_obj_set_width(g_status_lbl, 440);
    lv_obj_set_style_text_font(g_status_lbl, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(g_status_lbl, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(g_status_lbl, 20, 32);

    lv_obj_t *maint_btn = lv_btn_create(parent);
    lv_obj_set_size(maint_btn, 140, 32);
    lv_obj_set_pos(maint_btn, 20, 58);
    lv_obj_set_style_bg_color(maint_btn, UI_COLOR_WARN, 0);
    lv_obj_add_event_cb(maint_btn, maint_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ml = lv_label_create(maint_btn);
    lv_label_set_text(ml, "Manutencao");
    lv_obj_center(ml);

    lv_obj_t *mute_btn = lv_btn_create(parent);
    lv_obj_set_size(mute_btn, 140, 32);
    lv_obj_set_pos(mute_btn, 180, 58);
    lv_obj_set_style_bg_color(mute_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_add_event_cb(mute_btn, mute_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *mutel = lv_label_create(mute_btn);
    lv_label_set_text(mutel, "MUTE");
    lv_obj_center(mutel);

    lv_obj_t *ato_btn = lv_btn_create(parent);
    lv_obj_set_size(ato_btn, 140, 32);
    lv_obj_set_pos(ato_btn, 340, 58);
    lv_obj_set_style_bg_color(ato_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_add_event_cb(ato_btn, ato_clear_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *atol = lv_label_create(ato_btn);
    lv_label_set_text(atol, "Limpar ATO");
    lv_obj_center(atol);

    static const char *icons[] = {"\xe2\x9a\x99", "\xe2\x9f\xb9", "\xf0\x9f\x94\x8d", "\xe2\x9a\x99"};
    static const char *texts[] = {"Reset Fabrica", "Reboot", "Logs", "Wizard"};
    static int actions[] = {0, 1, 2, 3};

    static const int x_pos[2] = {15, 245};
    static const int y_pos[2] = {110, 190};

    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        ui_menu_tile_create(&sys_tiles[i], parent, x_pos[col], y_pos[row], icons[i], texts[i]);
        ui_menu_tile_set_callback(&sys_tiles[i], sys_tile_cb, &actions[i]);
    }

    ui_inline_hint_create(&g_hint, parent);
    ui_confirm_dialog_create(&g_confirm_dlg, parent,
        "Confirmar acao critica?");
    ui_confirm_dialog_set_confirm_cb(&g_confirm_dlg, confirm_ok_cb);
    ui_confirm_dialog_set_cancel_cb(&g_confirm_dlg, confirm_cancel_cb);
}

void ui_screen_system_update(ui_root_vm_t *vm)
{
    (void)vm;
    if (!g_status_lbl) return;
    char buf[96];
    uint64_t now_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);
    if (maintenance_mode_is_active()) {
        uint32_t rem = maintenance_mode_remaining_s(now_s);
        if (maintenance_mode_is_expiring_soon(now_s)) {
            snprintf(buf, sizeof(buf),
                     "Manutencao EXPIRA EM BREVE — restam %lu min",
                     (unsigned long)((rem + 59U) / 60U));
        } else {
            snprintf(buf, sizeof(buf), "Manutencao ATIVA — restam %lu min",
                     (unsigned long)(rem / 60U));
        }
    } else {
        snprintf(buf, sizeof(buf), "Manutencao inativa — automacoes normais");
    }
    lv_label_set_text(g_status_lbl, buf);
}
