#include "../ui_screens.h"
#include "global_state.h"
#include "plug_model.h"
#include "driver_acs712.h"

#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "screen_devices1";

static lv_obj_t *plug_rows[5];
static lv_obj_t *plug_name_labels[5];
static lv_obj_t *plug_state_btns[5];
static lv_obj_t *plug_current_labels[5];

extern global_state_t g_gs;

static const char *plug_names[5] = {"P01", "P02", "P03", "P04", "P05"};

static void screen_init_devices1(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Dispositivos 1/2");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);

    for (int i = 0; i < 5; i++) {
        plug_rows[i] = lv_obj_create(parent);
        lv_obj_set_size(plug_rows[i], 460, 50);
        lv_obj_set_style_border_width(plug_rows[i], 1, 0);
        lv_obj_set_style_radius(plug_rows[i], 3, 0);
        lv_obj_align(plug_rows[i], LV_ALIGN_TOP_LEFT, 10, 35 + i * 52);
        lv_obj_clear_flag(plug_rows[i], LV_OBJ_FLAG_SCROLLABLE);

        plug_name_labels[i] = lv_label_create(plug_rows[i]);
        lv_label_set_text(plug_name_labels[i], plug_names[i]);
        lv_obj_align(plug_name_labels[i], LV_ALIGN_LEFT_MID, 5, 0);

        plug_state_btns[i] = lv_btn_create(plug_rows[i]);
        lv_obj_set_size(plug_state_btns[i], 50, 30);
        lv_obj_align(plug_state_btns[i], LV_ALIGN_LEFT_MID, 60, 0);
        lv_obj_t *btn_label = lv_label_create(plug_state_btns[i]);
        lv_label_set_text(btn_label, "OFF");
        lv_obj_center(btn_label);

        plug_current_labels[i] = lv_label_create(plug_rows[i]);
        lv_label_set_text(plug_current_labels[i], "0.00 A");
        lv_obj_align(plug_current_labels[i], LV_ALIGN_LEFT_MID, 140, 0);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< >   Navegar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_devices1(void)
{
    for (int i = 0; i < 5; i++) {
        char buf[32];
        float cur = 0;
        acs712_read_plug((uint8_t)(i + 1), &cur);
        snprintf(buf, sizeof(buf), "%.2f A", (double)cur);
        lv_label_set_text(plug_current_labels[i], buf);

        lv_obj_t *btn_label = lv_obj_get_child(plug_state_btns[i], 0);
        bool safe_state = (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF);
        if (safe_state) {
            lv_label_set_text(btn_label, "OFF");
            lv_obj_set_style_bg_color(plug_state_btns[i], lv_color_make(100, 0, 0), 0);
            lv_obj_set_style_bg_color(plug_rows[i], lv_color_make(60, 0, 0), 0);
        } else {
            lv_label_set_text(btn_label, "OFF");
            lv_obj_set_style_bg_color(plug_state_btns[i], lv_color_make(100, 100, 100), 0);
            lv_obj_set_style_bg_color(plug_rows[i], lv_color_make(40, 40, 40), 0);
        }
    }
}

static void __attribute__((constructor)) register_devices1(void)
{
    ui_screen_register(SCREEN_DEVICES1, screen_init_devices1, screen_update_devices1);
}
