// @requirement RF-PLUG-001 a RF-PLUG-010 Tela de dispositivos P06-P10
#include "../ui_screens.h"
#include "global_state.h"
#include "plug_model.h"
#include "services/plug_manager.h"
#include "driver_acs712.h"
#include "lvgl.h"

static lv_obj_t *plug_rows[5];
static lv_obj_t *plug_icon_labels[5];
static lv_obj_t *plug_name_labels[5];
static lv_obj_t *plug_state_labels[5];
static lv_obj_t *plug_current_labels[5];
static lv_obj_t *plug_alert_labels[5];
static lv_obj_t *ato_status = NULL;

extern global_state_t g_gs;

static const char *plug_type_icon(plug_type_t t)
{
    switch (t) {
        case PLUG_TYPE_BOMBA:     return LV_SYMBOL_REFRESH;
        case PLUG_TYPE_AQUECEDOR: return LV_SYMBOL_WARNING;
        case PLUG_TYPE_COOLER:    return LV_SYMBOL_DOWNLOAD;
        case PLUG_TYPE_FILTRO:    return LV_SYMBOL_POWER;
        case PLUG_TYPE_AIREADOR:  return LV_SYMBOL_REFRESH;
        case PLUG_TYPE_LUZ:       return LV_SYMBOL_CHARGE;
        default:                  return LV_SYMBOL_OK;
    }
}

static lv_color_t plug_type_color(plug_type_t t)
{
    switch (t) {
        case PLUG_TYPE_BOMBA:     return lv_color_make(0, 100, 200);
        case PLUG_TYPE_AQUECEDOR: return lv_color_make(200, 50, 0);
        case PLUG_TYPE_COOLER:    return lv_color_make(0, 120, 180);
        case PLUG_TYPE_FILTRO:    return lv_color_make(60, 150, 60);
        case PLUG_TYPE_AIREADOR:  return lv_color_make(100, 180, 200);
        case PLUG_TYPE_LUZ:       return lv_color_make(200, 180, 0);
        default:                  return lv_color_make(100, 100, 100);
    }
}

static void screen_init_devices2(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Dispositivos 2/2");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    for (int i = 0; i < 5; i++) {
        plug_rows[i] = lv_obj_create(parent);
        lv_obj_set_size(plug_rows[i], 460, 38);
        lv_obj_set_style_radius(plug_rows[i], 4, 0);
        lv_obj_set_style_border_width(plug_rows[i], 0, 0);
        lv_obj_align(plug_rows[i], LV_ALIGN_TOP_LEFT, 10, 68 + i * 40);
        lv_obj_clear_flag(plug_rows[i], LV_OBJ_FLAG_SCROLLABLE);

        plug_icon_labels[i] = lv_label_create(plug_rows[i]);
        lv_label_set_text(plug_icon_labels[i], LV_SYMBOL_OK);
        lv_obj_set_style_bg_color(plug_icon_labels[i], lv_color_make(80, 80, 80), 0);
        lv_obj_set_style_radius(plug_icon_labels[i], 3, 0);
        lv_obj_set_style_pad_all(plug_icon_labels[i], 2, 0);
        lv_obj_align(plug_icon_labels[i], LV_ALIGN_LEFT_MID, 4, 0);

        plug_name_labels[i] = lv_label_create(plug_rows[i]);
        lv_label_set_text(plug_name_labels[i], "P0x");
        lv_obj_align(plug_name_labels[i], LV_ALIGN_LEFT_MID, 40, 0);
        lv_obj_set_style_text_font(plug_name_labels[i], &lv_font_montserrat_14, 0);

        plug_state_labels[i] = lv_label_create(plug_rows[i]);
        lv_label_set_text(plug_state_labels[i], "OFF");
        lv_obj_align(plug_state_labels[i], LV_ALIGN_LEFT_MID, 180, 0);
        lv_obj_set_style_text_font(plug_state_labels[i], &lv_font_montserrat_14, 0);

        plug_current_labels[i] = lv_label_create(plug_rows[i]);
        lv_label_set_text(plug_current_labels[i], "0.00 A");
        lv_obj_align(plug_current_labels[i], LV_ALIGN_LEFT_MID, 290, 0);
        lv_obj_set_style_text_font(plug_current_labels[i], &lv_font_montserrat_14, 0);

        plug_alert_labels[i] = lv_label_create(plug_rows[i]);
        lv_label_set_text(plug_alert_labels[i], "");
        lv_obj_align(plug_alert_labels[i], LV_ALIGN_RIGHT_MID, -4, 0);
    }

    ato_status = lv_label_create(parent);
    lv_label_set_text(ato_status, "ATO: --");
    lv_obj_align(ato_status, LV_ALIGN_BOTTOM_LEFT, 10, -42);
}

static void screen_update_devices2(void)
{
    for (int i = 0; i < 5; i++) {
        plug_id_t pid = (plug_id_t)(i + 6);
        plug_model_t *p = plug_manager_get(pid);
        if (!p) continue;

        char buf[32];

        lv_label_set_text(plug_icon_labels[i], plug_type_icon(p->type));
        lv_obj_set_style_bg_color(plug_icon_labels[i], plug_type_color(p->type), 0);
        lv_obj_set_style_text_color(plug_icon_labels[i], lv_color_make(255, 255, 255), 0);

        snprintf(buf, sizeof(buf), "P%02u %s", (unsigned)p->id, p->name);
        lv_label_set_text(plug_name_labels[i], buf);

        bool safe_state = (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF);
        bool is_on = !safe_state && p->effective_state;
        lv_label_set_text(plug_state_labels[i], is_on ? "ON" : "OFF");
        lv_obj_set_style_text_color(plug_state_labels[i],
            is_on ? lv_color_make(0, 200, 0) : lv_color_make(150, 150, 150), 0);

        float cur = 0;
        acs712_read_plug(pid, &cur);
        snprintf(buf, sizeof(buf), "%.2f A", (double)cur);
        lv_label_set_text(plug_current_labels[i], buf);

        if (p->bypass_detected) {
            lv_label_set_text(plug_alert_labels[i], LV_SYMBOL_WARNING);
            lv_obj_set_style_text_color(plug_alert_labels[i], lv_color_make(255, 50, 0), 0);
        } else {
            lv_label_set_text(plug_alert_labels[i], "");
        }
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "ATO: %s", g_gs.ato_ok ? "OK" : "FALHA");
    lv_label_set_text(ato_status, buf);
    lv_obj_set_style_text_color(ato_status,
        g_gs.ato_ok ? lv_color_make(0, 200, 0) : lv_color_make(255, 50, 50), 0);
}

static void __attribute__((constructor)) register_devices2(void)
{
    ui_screen_register(SCREEN_DEVICES2, screen_init_devices2, screen_update_devices2);
}
