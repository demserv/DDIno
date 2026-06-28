#include "ui_screen_ato.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "global_state.h"

extern global_state_t g_gs;

static void ato_back_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
}

static const char *state_label(ui_ato_state_t st)
{
    switch (st) {
        case UI_ATO_IDLE:      return "NORMAL";
        case UI_ATO_REFILLING: return "REENCHENDO";
        case UI_ATO_BLOCKED:   return "BLOQUEADO";
        case UI_ATO_ERROR:     return "ERRO";
        default:               return "DESCONHECIDO";
    }
}

static lv_obj_t *g_state_label = NULL;
static lv_obj_t *g_level_label = NULL;
static lv_obj_t *g_pump_label = NULL;

void ui_screen_ato_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Controle ATO");
    lv_obj_set_style_text_font(title, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    g_state_label = lv_label_create(parent);
    lv_label_set_text(g_state_label, "Estado: --");
    lv_obj_set_style_text_font(g_state_label, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(g_state_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(g_state_label, LV_ALIGN_TOP_LEFT, 20, 50);

    g_level_label = lv_label_create(parent);
    lv_label_set_text(g_level_label, "Nivel ADC: --");
    lv_obj_set_style_text_font(g_level_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(g_level_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(g_level_label, LV_ALIGN_TOP_LEFT, 20, 80);

    g_pump_label = lv_label_create(parent);
    lv_label_set_text(g_pump_label, "Bomba: --");
    lv_obj_set_style_text_font(g_pump_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(g_pump_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(g_pump_label, LV_ALIGN_TOP_LEFT, 20, 110);

    lv_obj_t *back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 100, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -15);
    lv_obj_set_style_bg_color(back_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(back_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(back_btn, ato_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Voltar");
    lv_obj_center(back_lbl);
}

void ui_screen_ato_update(ui_root_vm_t *vm)
{
    if (!vm) return;
    if (g_state_label) {
        lv_label_set_text(g_state_label, state_label(vm->ato.state));
    }
    if (g_level_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Nivel ADC: %ld", (long)vm->ato.level_adc);
        lv_label_set_text(g_level_label, buf);
    }
    if (g_pump_label) {
        lv_label_set_text(g_pump_label, vm->ato.pump_on ? "Bomba: LIGADA" : "Bomba: DESLIGADA");
    }
}
