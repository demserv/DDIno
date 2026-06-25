#include "../ui_screens.h"
#include "global_state.h"

#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "screen_submenu";

static lv_obj_t *submenu_labels[6];

static uint16_t g_config_mains_voltage = 127;
static float g_config_temp_min = 24.0f;
static float g_config_temp_max = 30.0f;
static uint16_t g_config_power_max_w = 1500;

static void submenu_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        intptr_t idx = (intptr_t)lv_obj_get_user_data(btn);
        if (idx == 0) {
            g_config_mains_voltage = (g_config_mains_voltage == 127) ? 220 : 127;
        }
    }
}

static void screen_init_submenu(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Configuracoes");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    const char *items[] = {
        "Tensao",
        "Temp Min",
        "Temp Max",
        "Potencia Max",
        "Data/Hora",
        "Wi-Fi"
    };

    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 440, 36);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 20, 50 + i * 40);
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn, submenu_btn_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, items[i]);
        lv_obj_center(lbl);

        submenu_labels[i] = lv_label_create(btn);
        lv_obj_align(submenu_labels[i], LV_ALIGN_RIGHT_MID, -10, 0);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< >   Voltar  |  Enter toggle");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_submenu(void)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d V", g_config_mains_voltage);
    lv_label_set_text(submenu_labels[0], buf);

    snprintf(buf, sizeof(buf), "%.1f C", (double)g_config_temp_min);
    lv_label_set_text(submenu_labels[1], buf);

    snprintf(buf, sizeof(buf), "%.1f C", (double)g_config_temp_max);
    lv_label_set_text(submenu_labels[2], buf);

    snprintf(buf, sizeof(buf), "%d W", g_config_power_max_w);
    lv_label_set_text(submenu_labels[3], buf);
}

static void __attribute__((constructor)) register_submenu(void)
{
    ui_screen_register(SCREEN_SUBMENU, screen_init_submenu, screen_update_submenu);
}
