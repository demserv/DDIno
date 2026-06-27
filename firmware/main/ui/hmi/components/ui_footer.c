// @requirement RF-UI-STATUS-001 Footer com estado global, uptime, alertas, página
#include "ui_footer.h"
#include "../ui_theme.h"
#include <stdio.h>

void ui_footer_create(ui_footer_t *footer, lv_obj_t *parent)
{
    footer->root = lv_obj_create(parent);
    lv_obj_set_size(footer->root, 480, 31);
    lv_obj_set_pos(footer->root, 0, 289);
    lv_obj_set_style_bg_color(footer->root, UI_COLOR_FOOTER, 0);
    lv_obj_set_style_border_width(footer->root, 0, 0);
    lv_obj_set_style_radius(footer->root, 0, 0);
    lv_obj_clear_flag(footer->root, LV_OBJ_FLAG_SCROLLABLE);

    footer->state_label = lv_label_create(footer->root);
    lv_label_set_text(footer->state_label, "NORMAL");
    lv_obj_set_style_text_font(footer->state_label, UI_FONT_NORMAL, 0);
    lv_obj_set_pos(footer->state_label, 8, 8);

    footer->uptime_label = lv_label_create(footer->root);
    lv_label_set_text(footer->uptime_label, "Uptime: 00h 00m");
    lv_obj_set_style_text_font(footer->uptime_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(footer->uptime_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(footer->uptime_label, 130, 8);

    footer->alerts_label = lv_label_create(footer->root);
    lv_label_set_text(footer->alerts_label, "Alertas: 0");
    lv_obj_set_style_text_font(footer->alerts_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(footer->alerts_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(footer->alerts_label, 310, 8);

    footer->page_label = lv_label_create(footer->root);
    lv_label_set_text(footer->page_label, "");
    lv_obj_set_style_text_font(footer->page_label, UI_FONT_NORMAL, 0);
    lv_obj_set_pos(footer->page_label, 395, 8);
}

void ui_footer_update(ui_footer_t *footer, const ui_footer_vm_t *vm)
{
    switch (vm->system_state) {
        case UI_SYSTEM_NORMAL:
            lv_label_set_text(footer->state_label, "NORMAL");
            lv_obj_set_style_text_color(footer->state_label, UI_COLOR_OK, 0);
            break;
        case UI_SYSTEM_DEGRADED:
            lv_label_set_text(footer->state_label, "DEGRADED");
            lv_obj_set_style_text_color(footer->state_label, UI_COLOR_WARN, 0);
            break;
        case UI_SYSTEM_SAFE_OFF:
            lv_label_set_text(footer->state_label, "SAFE_OFF");
            lv_obj_set_style_text_color(footer->state_label, UI_COLOR_CRITICAL, 0);
            break;
        case UI_SYSTEM_EMERGENCY:
            lv_label_set_text(footer->state_label, "EMERGENCY");
            lv_obj_set_style_text_color(footer->state_label, UI_COLOR_CRITICAL, 0);
            break;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "Uptime: %s", vm->uptime_text);
    lv_label_set_text(footer->uptime_label, buf);

    snprintf(buf, sizeof(buf), "Alertas: %u", (unsigned)vm->active_alerts_count);
    lv_label_set_text(footer->alerts_label, buf);
    if (vm->active_alerts_count > 0) {
        lv_obj_set_style_text_color(footer->alerts_label, UI_COLOR_WARN, 0);
    } else {
        lv_obj_set_style_text_color(footer->alerts_label, UI_COLOR_TEXT_MUTED, 0);
    }

    char page_buf[32] = "";
    uint8_t max_dots = vm->page_count;
    if (max_dots > 12) max_dots = 12;
    for (uint8_t i = 0; i < max_dots; i++) {
        if (i == vm->page_index) {
            page_buf[i] = '>';
        } else {
            page_buf[i] = '.';
        }
    }
    page_buf[max_dots] = '\0';
    lv_label_set_text(footer->page_label, page_buf);
}
