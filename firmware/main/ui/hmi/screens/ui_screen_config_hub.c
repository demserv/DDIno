// @requirement RF-UI-MENU-002 Hub Configuracoes
#include "ui_screen_config_hub.h"
#include "../ui_theme.h"
#include "../components/ui_menu_tile.h"
#include "../ui_screen_manager.h"
#include "../ui_events.h"

static ui_menu_tile_t tiles[8];

static void tile_cb(lv_event_t *e)
{
    ui_screen_id_t *scr = (ui_screen_id_t *)lv_event_get_user_data(e);
    if (scr) ui_screen_manager_show(*scr);
}

static void export_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_CONFIG_EXPORT);
}

static void import_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_CONFIG_IMPORT);
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
}

void ui_screen_config_hub_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Configuracoes");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_pos(title, 175, 6);

    static const char *icons[] = {
        "\xe2\x99\xa8", "\xe2\x99\xa8", "\xe2\x96\x88", "\xe2\x9a\xa1",
        "\xe2\x8f\xb1", "\xe2\x9a\x99", "\xf0\x9f\x93\x81", "\xf0\x9f\x93\x81"
    };
    static const char *texts[] = {
        "Temperatura", "ATO", "Por Plugue", "Calibracao",
        "Carrossel", "Perfis", "Export JSON", "Import JSON"
    };
    static ui_screen_id_t targets[] = {
        UI_SCREEN_CONFIG_TEMPERATURE,
        UI_SCREEN_ATO,
        UI_SCREEN_DEVICES_1,
        UI_SCREEN_CALIBRATION,
        UI_SCREEN_CAROUSEL_SETTINGS,
        UI_SCREEN_PROFILES,
        UI_SCREEN_CONFIG_HUB,
        UI_SCREEN_CONFIG_HUB
    };
    static const int col_x[3] = {15, 170, 325};
    static const int row_y[3] = {32, 100, 168};

    for (int i = 0; i < 8; i++) {
        int r = i / 3;
        int c = i % 3;
        ui_menu_tile_create(&tiles[i], parent, col_x[c], row_y[r], icons[i], texts[i]);
        if (i == 6) {
            ui_menu_tile_set_callback(&tiles[i], export_cb, NULL);
        } else if (i == 7) {
            ui_menu_tile_set_callback(&tiles[i], import_cb, NULL);
        } else {
            ui_menu_tile_set_callback(&tiles[i], tile_cb, &targets[i]);
        }
    }

    lv_obj_t *back = lv_btn_create(parent);
    lv_obj_set_size(back, 100, 28);
    lv_obj_set_pos(back, 10, 250);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "Voltar");
    lv_obj_center(bl);
}

void ui_screen_config_hub_update(ui_root_vm_t *vm)
{
    (void)vm;
}
