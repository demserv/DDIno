// @requirement RF-PLUG-001 a RF-PLUG-014 Linha de dispositivo com ícone, nome, estado, corrente
#include "ui_device_row.h"
#include "../ui_theme.h"
#include "../ui_preset_picker.h"
#include "plug_preset_catalog.h"
#include "plug_model.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_preset_parent = NULL;

static const char *get_plug_icon(const ui_plug_vm_t *plug)
{
    return plug_preset_icon_for_name(plug->name);
}

static lv_color_t get_plug_state_color(ui_plug_state_t state)
{
    switch (state) {
        case UI_PLUG_ON: return UI_COLOR_OK;
        case UI_PLUG_OFF: return UI_COLOR_DISABLED;
        case UI_PLUG_BLOCKED: return UI_COLOR_CRITICAL;
        case UI_PLUG_ERROR: return UI_COLOR_CRITICAL;
    }
    return UI_COLOR_TEXT_DIM;
}

static const char *get_plug_state_text(ui_plug_state_t state)
{
    switch (state) {
        case UI_PLUG_ON: return "ON";
        case UI_PLUG_OFF: return "OFF";
        case UI_PLUG_BLOCKED: return "BLOCKED";
        case UI_PLUG_ERROR: return "ERROR";
    }
    return "?";
}

static void preset_btn_cb(lv_event_t *e)
{
    uint8_t *pid = (uint8_t *)lv_event_get_user_data(e);
    if (pid && s_preset_parent && *pid >= 3) {
        ui_preset_picker_show(s_preset_parent, (plug_id_t)(*pid));
    }
}

void ui_device_row_set_preset_parent(lv_obj_t *parent)
{
    s_preset_parent = parent;
}

void ui_device_row_create(ui_device_row_t *row, lv_obj_t *parent, int x, int y, uint8_t plug_id)
{
    static uint8_t plug_ids[10];

    row->root = lv_obj_create(parent);
    lv_obj_set_size(row->root, 460, 42);
    lv_obj_set_pos(row->root, x, y);
    lv_obj_set_style_bg_color(row->root, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(row->root, UI_RADIUS_CARD, 0);
    lv_obj_set_style_border_width(row->root, 0, 0);
    lv_obj_clear_flag(row->root, LV_OBJ_FLAG_SCROLLABLE);

    row->icon_label = lv_label_create(row->root);
    lv_label_set_text(row->icon_label, "?");
    lv_obj_set_style_text_font(row->icon_label, UI_FONT_MEDIUM, 0);
    lv_obj_set_pos(row->icon_label, 14, 10);

    row->code_label = lv_label_create(row->root);
    lv_label_set_text(row->code_label, "P00");
    lv_obj_set_style_text_font(row->code_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(row->code_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(row->code_label, 50, 4);

    row->name_label = lv_label_create(row->root);
    lv_label_set_text(row->name_label, "");
    lv_obj_set_style_text_font(row->name_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(row->name_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(row->name_label, 50, 19);

    row->tag_label = lv_label_create(row->root);
    lv_label_set_text(row->tag_label, "");
    lv_obj_set_style_text_font(row->tag_label, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(row->tag_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(row->tag_label, 50, 19);

    row->state_label = lv_label_create(row->root);
    lv_label_set_text(row->state_label, "OFF");
    lv_obj_set_style_text_font(row->state_label, UI_FONT_NORMAL, 0);
    lv_obj_set_pos(row->state_label, 310, 13);

    row->current_label = lv_label_create(row->root);
    lv_label_set_text(row->current_label, "-- A");
    lv_obj_set_style_text_font(row->current_label, UI_FONT_NORMAL, 0);
    lv_obj_set_pos(row->current_label, 380, 13);

    row->preset_btn = lv_btn_create(row->root);
    lv_obj_set_size(row->preset_btn, 36, 24);
    lv_obj_set_pos(row->preset_btn, 430, 9);
    lv_obj_set_style_bg_color(row->preset_btn, UI_COLOR_PANEL, 0);
    lv_obj_t *pl = lv_label_create(row->preset_btn);
    lv_label_set_text(pl, "P");
    lv_obj_center(pl);

    plug_ids[plug_id - 1] = plug_id;
    lv_obj_add_event_cb(row->preset_btn, preset_btn_cb, LV_EVENT_CLICKED, &plug_ids[plug_id - 1]);
    if (plug_id < 3) {
        lv_obj_add_flag(row->preset_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_device_row_update(ui_device_row_t *row, const ui_plug_vm_t *plug)
{
    lv_label_set_text(row->icon_label, get_plug_icon(plug));
    lv_label_set_text(row->code_label, plug->code);
    lv_label_set_text(row->name_label, plug->name);

    if (plug->role_tag[0] != '\0') {
        lv_obj_set_pos(row->tag_label, 50 + lv_obj_get_width(row->name_label) + 8, 19);
        char tag_buf[16];
        snprintf(tag_buf, sizeof(tag_buf), "[%s]", plug->role_tag);
        lv_label_set_text(row->tag_label, tag_buf);
        lv_obj_clear_flag(row->tag_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(row->tag_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_set_style_text_color(row->state_label, get_plug_state_color(plug->state), 0);
    lv_label_set_text(row->state_label, get_plug_state_text(plug->state));

    if (plug->current_valid) {
        char cur_buf[16];
        snprintf(cur_buf, sizeof(cur_buf), "%.2f A", (double)plug->current_a);
        lv_label_set_text(row->current_label, cur_buf);
        lv_obj_set_style_text_color(row->current_label, UI_COLOR_YELLOW, 0);
    } else {
        lv_label_set_text(row->current_label, "-- A");
        lv_obj_set_style_text_color(row->current_label, UI_COLOR_DISABLED, 0);
    }
}
