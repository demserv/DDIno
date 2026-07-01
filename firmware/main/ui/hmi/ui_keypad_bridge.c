// @requirement RF-UI-SHORTCUT-001 / RF-UI-MUTE-001 Cabeamento keypad → eventos UI
#include "ui_app.h"
#include "ui_events.h"
#include "driver_ad_keypad_lvgl.h"
#include "components/ui_inline_hint.h"
#include "lvgl.h"

static ui_inline_hint_t s_hint;
static bool s_hint_created = false;

static void ensure_hint(void)
{
    if (!s_hint_created) {
        ui_inline_hint_create(&s_hint, lv_layer_top());
        s_hint_created = true;
    }
}

static void on_keypad_special(int key)
{
    switch (key) {
        case LV_KEY_FEED:
            ui_events_emit(UI_EVENT_REQUEST_FEED_MODE);
            ensure_hint();
            ui_inline_hint_show(&s_hint, "Atalho: Feed Mode", 2000);
            break;
        case LV_KEY_HOME:
            ui_events_emit(UI_EVENT_NAVIGATE_HOME);
            ensure_hint();
            ui_inline_hint_show(&s_hint, "Atalho: Home", 1500);
            break;
        case LV_KEY_MUTE:
            ui_events_emit(UI_EVENT_REQUEST_MUTE);
            ensure_hint();
            ui_inline_hint_show(&s_hint, "MUTE ativado", 2000);
            break;
        default:
            break;
    }
}

void ui_app_register_keypad(void)
{
    driver_ad_keypad_set_special_cb(on_keypad_special);
}
