// @requirement RF-RESET-001 a RF-RESET-004 Tela de sistema com reset, wizard, informações
// @requirement RF-UI-MUTE-001 Seletor de duracao do MUTE (5/10/15/até-ACK)
#include "ui_screen_system.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include "../components/ui_menu_tile.h"
#include "../ui_screen_manager.h"
#include "services/config_manager.h"
#include "services/audit_log.h"

static ui_menu_tile_t sys_tiles[4];
static lv_obj_t *s_mute_roller = NULL;

static void mute_roller_cb(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target(e);
    uint16_t sel = lv_roller_get_selected(roller);
    if (sel > UI_MUTE_DURATION_UNTIL_ACK) sel = UI_MUTE_DURATION_5MIN;
    ui_events_set_mute_duration((ui_mute_duration_t)sel);
    config_set_mute_duration((uint8_t)sel);
    audit_log_event(AUDIT_CONFIG_CHANGE, "mute duration updated via UI");
}

static void sys_tile_cb(lv_event_t *e)
{
    int *action = (int *)lv_event_get_user_data(e);
    if (!action) return;
    switch (*action) {
        case 0:
            ui_screen_manager_show(UI_SCREEN_DIAGNOSTICS);
            break;
        case 1:
            ui_screen_manager_show(UI_SCREEN_WIZARD);
            break;
        case 2:
            ui_events_emit(UI_EVENT_REQUEST_SAFE_REBOOT);
            break;
        case 3:
            ui_screen_manager_show(UI_SCREEN_LOGS);
            break;
    }
}

void ui_screen_system_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Sistema");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 195, 6);

    static const char *icons[] = {"\xe2\x9a\x95", "\xe2\x9a\x99", "\xe2\x9f\xb9", "\xe2\x84\xb9"};
    static const char *texts[] = {"Diagnostico", "Manutencao", "Reset Seguro", "Ver logs"};
    static const int actions[] = {0, 1, 2, 3};

    static const int x_pos[2] = {15, 245};
    static const int y_pos[2] = {40, 130};

    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        ui_menu_tile_create(&sys_tiles[i], parent, x_pos[col], y_pos[row], icons[i], texts[i]);
        ui_menu_tile_set_callback(&sys_tiles[i], sys_tile_cb, &actions[i]);
    }

    lv_obj_t *mute_lbl = lv_label_create(parent);
    lv_label_set_text(mute_lbl, "Duracao MUTE:");
    lv_obj_set_style_text_font(mute_lbl, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(mute_lbl, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(mute_lbl, 15, 228);

    s_mute_roller = lv_roller_create(parent);
    lv_roller_set_options(s_mute_roller, "5 min\n10 min\n15 min\nAte ACK", LV_ROLLER_MODE_NORMAL);
    lv_obj_set_size(s_mute_roller, 200, 60);
    lv_obj_set_pos(s_mute_roller, 130, 218);
    lv_roller_set_selected(s_mute_roller, config_get_mute_duration(), LV_ANIM_OFF);
    lv_obj_add_event_cb(s_mute_roller, mute_roller_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void ui_screen_system_update(ui_root_vm_t *vm)
{
    (void)vm;
    if (s_mute_roller) {
        lv_roller_set_selected(s_mute_roller, config_get_mute_duration(), LV_ANIM_OFF);
    }
}
