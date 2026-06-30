// @requirement RF-RESET-001 a RF-RESET-004 Tela de sistema com reset, wizard, informações
// @requirement RF-UI-WIZARD-001..005 Acesso ao wizard
#include "ui_screen_system.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include "../components/ui_menu_tile.h"
#include "../ui_screen_manager.h"

static ui_menu_tile_t sys_tiles[4];

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
    static const char *texts[] = {"Diagnostico", "Manutencao", "Reset Seguro", "Ver does"};
    static const int actions[] = {0, 1, 2, 3};

    static const int x_pos[2] = {15, 245};
    static const int y_pos[2] = {40, 130};

    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        ui_menu_tile_create(&sys_tiles[i], parent, x_pos[col], y_pos[row], icons[i], texts[i]);
        ui_menu_tile_set_callback(&sys_tiles[i], sys_tile_cb, &actions[i]);
    }
}

void ui_screen_system_update(ui_root_vm_t *vm)
{
    (void)vm;
}
