#include "ui_screen_wizard.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "global_state.h"
#include "config_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "wizard";

extern global_state_t g_gs;

static lv_obj_t *g_step_title = NULL;
static lv_obj_t *g_step_desc = NULL;
static lv_obj_t *g_step_content = NULL;
static lv_obj_t *g_back_btn = NULL;
static lv_obj_t *g_next_btn = NULL;
static lv_obj_t *g_next_label = NULL;

static const char *step_name(uint8_t step)
{
    switch (step) {
        case 0: return "Boas-vindas";
        case 1: return "Senha Admin";
        case 2: return "Temperatura";
        case 3: return "ATO";
        case 4: return "Eletrica";
        case 5: return "Revisao Final";
        default: return "Concluido";
    }
}

static void render_step(uint8_t step)
{
    if (!g_step_desc || !g_step_content) return;

    lv_obj_clean(g_step_content);
    lv_label_set_text(g_step_title, step_name(step));

    switch (step) {
        case 0: {
            lv_label_set_text(g_step_desc, "Bem-vindo ao Monitor de Aquario Inteligente.\nConfigure os parametros basicos para comecar.");
            break;
        }
        case 1: {
            lv_label_set_text(g_step_desc, "A senha admin e definida via API REST.\nConsulte o manual para configuracao.");
            break;
        }
        case 2: {
            const thermal_params_storage_t *tp = config_get_thermal();
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "Temperatura:\nNormal: %.1f C\nCritica: %.1f C\nExtrema: %.1f C\nHisterese: %.1f C",
                     (double)tp->temp_normal_c, (double)tp->temp_critical_c,
                     (double)tp->temp_extreme_c, (double)tp->hysteresis_c);
            lv_label_set_text(g_step_desc, buf);
            break;
        }
        case 3: {
            const ato_params_storage_t *ap = config_get_ato();
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "ATO:\nNivel baixo ADC: %ld\nNivel alto ADC: %ld\nMargem overflow: %ld\nTimeout: %lu s",
                     (long)ap->low_level_adc, (long)ap->high_level_adc,
                     (long)ap->overflow_margin_adc, (unsigned long)ap->refill_timeout_s);
            lv_label_set_text(g_step_desc, buf);
            break;
        }
        case 4: {
            const electric_params_storage_t *ep = config_get_electric();
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "Eletrica:\nPotencia total: %.0f W\nCorrente por tomada: %.1f A\nSobretensao: %.0f V\nSubtensao: %.0f V",
                     (double)ep->total_power_limit_w, (double)ep->per_plug_current_limit_a,
                     (double)ep->overvoltage_limit_v, (double)ep->undervoltage_limit_v);
            lv_label_set_text(g_step_desc, buf);
            break;
        }
        case 5: {
            lv_label_set_text(g_step_desc, "Revise as configuracoes acima.\nConfirme para ativar o sistema.");
            break;
        }
        default:
            lv_label_set_text(g_step_desc, "Wizard concluido!");
            break;
    }
}

static void wizard_back_cb(lv_event_t *e)
{
    (void)e;
    uint8_t step = config_get_wizard_step();
    if (step == 0 || step >= WIZARD_STEP_COMPLETE) {
        ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
        return;
    }
    config_set_wizard_step(step - 1);
    render_step(step - 1);
    g_gs.wizard_step = (wizard_step_t)(step - 1);
}

static void wizard_next_cb(lv_event_t *e)
{
    (void)e;
    uint8_t step = config_get_wizard_step();
    if (step >= WIZARD_STEP_COMPLETE) {
        ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
        return;
    }

    if (step == 5) {
        config_set_wizard_completed(true);
        config_set_wizard_step(WIZARD_STEP_COMPLETE);
        g_gs.wizard_completed = true;
        g_gs.wizard_step = WIZARD_STEP_COMPLETE;
        ESP_LOGI(TAG, "Wizard concluido, sistema liberado");
        ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
        return;
    }

    config_set_wizard_step(step + 1);
    g_gs.wizard_step = (wizard_step_t)(step + 1);
    render_step(step + 1);
}

void ui_screen_wizard_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Wizard Inicial");
    lv_obj_set_style_text_font(title, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    g_step_title = lv_label_create(parent);
    lv_obj_set_style_text_font(g_step_title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(g_step_title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(g_step_title, LV_ALIGN_TOP_LEFT, 20, 45);

    g_step_desc = lv_label_create(parent);
    lv_obj_set_width(g_step_desc, 440);
    lv_obj_set_style_text_font(g_step_desc, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(g_step_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(g_step_desc, LV_ALIGN_TOP_LEFT, 20, 75);

    g_step_content = lv_obj_create(parent);
    lv_obj_set_size(g_step_content, 440, 160);
    lv_obj_set_pos(g_step_content, 20, 130);
    lv_obj_set_style_border_width(g_step_content, 0, 0);
    lv_obj_set_style_bg_opa(g_step_content, LV_OPA_TRANSP, 0);

    g_back_btn = lv_btn_create(parent);
    lv_obj_set_size(g_back_btn, 100, 28);
    lv_obj_align(g_back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -15);
    lv_obj_set_style_bg_color(g_back_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(g_back_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(g_back_btn, wizard_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(g_back_btn);
    lv_label_set_text(back_lbl, "Voltar");
    lv_obj_center(back_lbl);

    g_next_btn = lv_btn_create(parent);
    lv_obj_set_size(g_next_btn, 100, 28);
    lv_obj_align(g_next_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lv_obj_set_style_bg_color(g_next_btn, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(g_next_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(g_next_btn, wizard_next_cb, LV_EVENT_CLICKED, NULL);
    g_next_label = lv_label_create(g_next_btn);
    lv_label_set_text(g_next_label, "Avancar");
    lv_obj_center(g_next_label);

    render_step(config_get_wizard_step());
}

void ui_screen_wizard_update(ui_root_vm_t *vm)
{
    (void)vm;
    render_step(config_get_wizard_step());
}
