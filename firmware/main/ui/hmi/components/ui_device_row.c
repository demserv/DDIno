#include "ui_device_row.h"
#include "../ui_theme.h"
#include <stdio.h>
#include <string.h>

static const char *get_plug_icon(const ui_plug_vm_t *plug)
{
    const char *name = plug->name;
    if (strstr(name, "Aquecedor")) return "\xe2\x96\xb2";
    if (strstr(name, "Cooler")) return "\xe2\x9c\x95";
    if (strstr(name, "Bomba")) return "\xe2\x97\x89";
    if (strstr(name, "Iluminacao")) return "\xe2\x98\x80";
    if (strstr(name, "UV") || strstr(name, "Esterilizador")) return "UV";
    if (strstr(name, "Reserva")) return "\xe2\x96\xa1";
    return "\xe2\x97\x8f";
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

void ui_device_row_create(ui_device_row_t *row, lv_obj_t *parent, int x, int y)
{
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
    lv_obj_set_pos(row->state_label, 350, 13);

    row->current_label = lv_label_create(row->root);
    lv_label_set_text(row->current_label, "-- A");
    lv_obj_set_style_text_font(row->current_label, UI_FONT_NORMAL, 0);
    lv_obj_set_pos(row->current_label, 420, 13);
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
