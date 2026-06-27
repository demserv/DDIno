// @requirement RF-UI-DIAG-001 Tela de diagnóstico com 9 subsistemas
// @requirement RF-FLOW-BOOT-003 Exibição de self-test na UI
#include "ui_screen_diagnostics.h"
#include "../ui_theme.h"
#include <stdio.h>

typedef struct {
    lv_obj_t *name_label;
    lv_obj_t *status_label;
} diag_row_t;

static diag_row_t rows[9];

static lv_color_t health_color(ui_health_state_t state)
{
    switch (state) {
        case UI_HEALTH_OK: return UI_COLOR_OK;
        case UI_HEALTH_DEGRADED: return UI_COLOR_WARN;
        case UI_HEALTH_FAILED: return UI_COLOR_CRITICAL;
        case UI_HEALTH_UNKNOWN:
        default: return UI_COLOR_TEXT_DIM;
    }
}

static const char *health_text(ui_health_state_t state)
{
    switch (state) {
        case UI_HEALTH_OK: return "OK";
        case UI_HEALTH_DEGRADED: return "DEGRADED";
        case UI_HEALTH_FAILED: return "FAILED";
        case UI_HEALTH_UNKNOWN:
        default: return "UNKNOWN";
    }
}

void ui_screen_diagnostics_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Diagnostico");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 178, 6);

    static const char *names[] = {
        "Temperatura", "ATO", "Energia", "Plugues",
        "Persistencia", "Seguranca", "Self-test",
        "Barramentos", "I/O"
    };

    static const int y_pos[9] = {32, 58, 84, 110, 136, 162, 188, 214, 240};
    for (int i = 0; i < 9; i++) {
        lv_obj_t *panel = lv_obj_create(parent);
        lv_obj_set_size(panel, 460, 24);
        lv_obj_set_pos(panel, 10, y_pos[i]);
        lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
        lv_obj_set_style_radius(panel, UI_RADIUS_SMALL, 0);
        lv_obj_set_style_border_width(panel, 0, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        rows[i].name_label = lv_label_create(panel);
        lv_label_set_text(rows[i].name_label, names[i]);
        lv_obj_set_style_text_font(rows[i].name_label, UI_FONT_NORMAL, 0);
        lv_obj_set_style_text_color(rows[i].name_label, UI_COLOR_TEXT_MAIN, 0);
        lv_obj_set_pos(rows[i].name_label, 14, 4);

        rows[i].status_label = lv_label_create(panel);
        lv_label_set_text(rows[i].status_label, "UNKNOWN");
        lv_obj_set_style_text_font(rows[i].status_label, UI_FONT_NORMAL, 0);
        lv_obj_set_pos(rows[i].status_label, 390, 4);
    }

    ui_screen_diagnostics_update(vm);
}

void ui_screen_diagnostics_update(ui_root_vm_t *vm)
{
    if (!vm) return;

    ui_health_state_t states[9] = {
        vm->diagnostics.temperature,
        vm->diagnostics.ato,
        vm->diagnostics.energy,
        vm->diagnostics.plugs,
        vm->diagnostics.persistence,
        vm->diagnostics.security,
        vm->diagnostics.selftest,
        vm->diagnostics.buses,
        vm->diagnostics.io
    };

    for (int i = 0; i < 9; i++) {
        lv_label_set_text(rows[i].status_label, health_text(states[i]));
        lv_obj_set_style_text_color(rows[i].status_label, health_color(states[i]), 0);
    }
}
