#include "../ui_screens.h"
#include "global_state.h"
#include "fsm/feed_fsm.h"

#include "lvgl.h"
#include "esp_log.h"

extern bool g_feed_request;

static const char *TAG = "screen_menu";

static lv_obj_t *menu_btns[6];
static lv_obj_t *feed_label = NULL;

extern global_state_t g_gs;

static void menu_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        intptr_t idx = (intptr_t)lv_obj_get_user_data(btn);
        switch (idx) {
            case 0:
                ui_screen_show(SCREEN_SUBMENU);
                break;
            case 1:
                ui_screen_show(SCREEN_ALERTS);
                break;
            case 2:
                if (!g_gs.feed_active) {
                    g_feed_request = true;
                }
                break;
            default:
                break;
        }
    }
}

static void screen_init_menu(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Menu");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    const char *items[] = {
        "Configuracoes",
        "Alarmes",
        "Feed Mode",
        "Manutencao",
        "Wizard",
        "Sobre"
    };

    for (int i = 0; i < 6; i++) {
        menu_btns[i] = lv_btn_create(parent);
        lv_obj_set_size(menu_btns[i], 440, 36);
        lv_obj_align(menu_btns[i], LV_ALIGN_TOP_LEFT, 20, 50 + i * 40);
        lv_obj_set_user_data(menu_btns[i], (void *)(intptr_t)i);
        lv_obj_add_event_cb(menu_btns[i], menu_btn_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(menu_btns[i]);
        lv_label_set_text(lbl, items[i]);
        lv_obj_center(lbl);
    }

    feed_label = lv_label_create(parent);
    lv_label_set_text(feed_label, "Feed Mode: Desligado");
    lv_obj_align(feed_label, LV_ALIGN_BOTTOM_LEFT, 20, -30);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< >   Enter p/ selecionar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_menu(void)
{
    char buf[64];
    if (g_gs.feed_active) {
        snprintf(buf, sizeof(buf), "Feed Mode: ATIVO (%lu s restantes)", (unsigned long)g_gs.feed_remaining_s);
    } else if (g_gs.feed_state == FEED_STATE_COOLDOWN) {
        snprintf(buf, sizeof(buf), "Feed Mode: Cooldown");
    } else {
        snprintf(buf, sizeof(buf), "Feed Mode: Desligado");
    }
    lv_label_set_text(feed_label, buf);

    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        for (int i = 0; i < 6; i++) {
            lv_obj_add_state(menu_btns[i], LV_STATE_DISABLED);
        }
    } else {
        for (int i = 0; i < 6; i++) {
            lv_obj_clear_state(menu_btns[i], LV_STATE_DISABLED);
        }
    }
}

static void __attribute__((constructor)) register_menu(void)
{
    ui_screen_register(SCREEN_MENU, screen_init_menu, screen_update_menu);
}
