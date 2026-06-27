// @requirement RF-UI-CAROUSEL-001 Tela de menu principal com navegação
#include "../ui_screens.h"
#include "global_state.h"
#include "lvgl.h"

extern global_state_t g_gs;

#define MENU_COLS 3
#define MENU_ROWS 3

static lv_obj_t *menu_btns[MENU_ROWS * MENU_COLS];

static struct { const char *icon; const char *label; screen_id_t target; } menu_items[] = {
    {LV_SYMBOL_HOME,       "Temp.",    SCREEN_DASHBOARD},
    {LV_SYMBOL_REFRESH,    "ATO",      SCREEN_DASHBOARD},
    {LV_SYMBOL_POWER,      "Energia",  SCREEN_ENERGY},
    {LV_SYMBOL_LIST,       "Dispos.",  SCREEN_DEVICES1},
    {LV_SYMBOL_BELL,       "Alertas",  SCREEN_ALERTS},
    {LV_SYMBOL_SETTINGS,   "Config.",  SCREEN_SUBMENU},
    {LV_SYMBOL_WIFI,       "Rede",     SCREEN_DASHBOARD},
    {LV_SYMBOL_REFRESH,    "Sistema",  SCREEN_DIAGNOSTIC},
    {LV_SYMBOL_WARNING,    "Calib.",   SCREEN_CALIBRATION},
};
#define MENU_ITEM_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

static void menu_btn_cb(lv_event_t *e)
{
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) return;
    lv_obj_t *btn = lv_event_get_target(e);
    intptr_t idx = (intptr_t)lv_obj_get_user_data(btn);
    if (idx >= 0 && idx < (intptr_t)MENU_ITEM_COUNT) {
        ui_screen_show(menu_items[idx].target);
    }
}

static void screen_init_menu(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Menu Principal");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    int btn_w = 140;
    int btn_h = 65;
    int gap_x = 15;
    int gap_y = 12;
    int start_x = (480 - (MENU_COLS * btn_w + (MENU_COLS - 1) * gap_x)) / 2;
    int start_y = 70;

    for (int i = 0; i < (int)MENU_ITEM_COUNT; i++) {
        int row = i / MENU_COLS;
        int col = i % MENU_COLS;

        menu_btns[i] = lv_btn_create(parent);
        lv_obj_set_size(menu_btns[i], btn_w, btn_h);
        lv_obj_set_style_radius(menu_btns[i], 6, 0);
        lv_obj_set_style_border_width(menu_btns[i], 0, 0);
        lv_obj_set_style_shadow_width(menu_btns[i], 4, 0);
        lv_obj_set_style_shadow_color(menu_btns[i], lv_color_make(0, 0, 0), 0);

        lv_obj_set_style_bg_color(menu_btns[i], lv_color_make(25, 30, 50), 0);
        lv_obj_set_style_bg_color(menu_btns[i], lv_color_make(50, 60, 90), LV_STATE_PRESSED);

        lv_obj_align(menu_btns[i], LV_ALIGN_TOP_LEFT,
                     start_x + col * (btn_w + gap_x),
                     start_y + row * (btn_h + gap_y));

        lv_obj_set_user_data(menu_btns[i], (void *)(intptr_t)i);
        lv_obj_add_event_cb(menu_btns[i], menu_btn_cb, LV_EVENT_CLICKED, NULL);

        // Icon on top, label below
        lv_obj_t *icon_lbl = lv_label_create(menu_btns[i]);
        lv_label_set_text(icon_lbl, menu_items[i].icon);
        lv_obj_align(icon_lbl, LV_ALIGN_TOP_MID, 0, 6);
        lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_16, 0);

        lv_obj_t *txt_lbl = lv_label_create(menu_btns[i]);
        lv_label_set_text(txt_lbl, menu_items[i].label);
        lv_obj_align(txt_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);
        lv_obj_set_style_text_font(txt_lbl, &lv_font_montserrat_14, 0);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< > Navegar | Enter selecionar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -42);
    lv_obj_set_style_text_color(hint, lv_color_make(120, 120, 120), 0);
}

static void screen_update_menu(void)
{
    bool disabled = (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF);
    for (int i = 0; i < (int)MENU_ITEM_COUNT; i++) {
        if (disabled) {
            lv_obj_add_state(menu_btns[i], LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(menu_btns[i], LV_STATE_DISABLED);
        }
    }
}

static void __attribute__((constructor)) register_menu(void)
{
    ui_screen_register(SCREEN_MENU, screen_init_menu, screen_update_menu);
}
