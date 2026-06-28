// @requirement RF-UI-CAROUSEL-001 Menu principal com 9 tiles em grid 3x3
#include "ui_screen_main_menu.h"
#include "../ui_theme.h"
#include "../components/ui_menu_tile.h"
#include "../ui_screen_manager.h"

static ui_menu_tile_t tiles[9];

static void tile_click_cb(lv_event_t *e)
{
    ui_screen_id_t *screen = (ui_screen_id_t *)lv_event_get_user_data(e);
    if (screen) {
        ui_screen_manager_show(*screen);
    }
}

void ui_screen_main_menu_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Menu Principal");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 178, 6);

    static const char *icons[] = {"\xe2\x99\xa8", "\xe2\x99\xa8", "\xe2\x9a\xa1", "\xe2\x96\x88", "\xe2\x9a\xa0", "\xe2\x9a\x99", "\xf0\x9f\x93\xa1", "\xe2\x9a\x99", "\xf0\x9f\x94\x8d"};
    static const char *texts[] = {"Temperatura", "ATO", "Energia", "Dispositivos", "Alertas", "Configuracao", "Rede/WiFi", "Sistema", "Logs"};

    static ui_screen_id_t targets[] = {
        UI_SCREEN_CONFIG_TEMPERATURE,
        UI_SCREEN_ATO,
        UI_SCREEN_ENERGY,
        UI_SCREEN_DEVICES_1,
        UI_SCREEN_ALERTS,
        UI_SCREEN_CONFIG_TEMPERATURE,
        UI_SCREEN_SYSTEM,
        UI_SCREEN_SYSTEM,
        UI_SCREEN_LOGS
    };

    static const int col_x[3] = {15, 170, 325};
    static const int row_y[3] = {28, 106, 184};

    int idx = 0;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            ui_menu_tile_create(&tiles[idx], parent, col_x[c], row_y[r], icons[idx], texts[idx]);
            ui_menu_tile_set_callback(&tiles[idx], tile_click_cb, &targets[idx]);
            idx++;
        }
    }
}

void ui_screen_main_menu_update(ui_root_vm_t *vm)
{
    (void)vm;
}
