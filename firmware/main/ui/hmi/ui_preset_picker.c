#include "ui_preset_picker.h"
#include "ui_theme.h"
#include "services/plug_manager.h"
#include "services/plug_preset_catalog.h"
#include "services/audit_log.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_overlay = NULL;
static plug_id_t s_plug_id = PLUG_ID_P01;

static void preset_close(lv_event_t *e)
{
    (void)e;
    if (s_overlay) {
        lv_obj_del(s_overlay);
        s_overlay = NULL;
    }
}

static void preset_apply_cb(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target(e);
    uint16_t sel = lv_roller_get_selected(roller);
    if (sel >= plug_preset_count()) {
        preset_close(e);
        return;
    }
    const plug_preset_t *p = plug_preset_get(sel);
    if (p && s_plug_id >= PLUG_ID_P03) {
        plug_manager_apply_preset(s_plug_id, p->id);
        audit_log_event(AUDIT_CONFIG_CHANGE, "plug preset applied via UI");
    }
    preset_close(e);
}

void ui_preset_picker_show(lv_obj_t *parent, plug_id_t plug_id)
{
    if (plug_id < PLUG_ID_P03) return;
    if (s_overlay) {
        lv_obj_del(s_overlay);
        s_overlay = NULL;
    }
    s_plug_id = plug_id;

    s_overlay = lv_obj_create(parent);
    lv_obj_set_size(s_overlay, 480, 320);
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_60, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_overlay);
    lv_obj_set_size(panel, 320, 180);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(panel, UI_RADIUS_CARD, 0);

    char title[48];
    snprintf(title, sizeof(title), "Preset P%02u", (unsigned)plug_id);
    lv_obj_t *lbl = lv_label_create(panel);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, UI_FONT_MEDIUM, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *roller = lv_roller_create(panel);
    {
        char opts[256] = "";
        for (uint8_t i = 0; i < plug_preset_count(); i++) {
            const plug_preset_t *p = plug_preset_get(i);
            if (!p) continue;
            if (opts[0] != '\0') strncat(opts, "\n", sizeof(opts) - strlen(opts) - 1);
            strncat(opts, p->name, sizeof(opts) - strlen(opts) - 1);
        }
        lv_roller_set_options(roller, opts, LV_ROLLER_MODE_NORMAL);
    }
    lv_obj_set_size(roller, 280, 90);
    lv_obj_align(roller, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(roller, preset_apply_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *btn = lv_btn_create(panel);
    lv_obj_set_size(btn, 80, 28);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(btn, preset_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Fechar");
    lv_obj_center(btn_lbl);
}
