#include "ui_screen_wizard.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "global_state.h"
#include "config_manager.h"
#include "param_catalog.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "wizard";

extern global_state_t g_gs;

static lv_obj_t *g_step_title = NULL;
static lv_obj_t *g_step_desc = NULL;
static lv_obj_t *g_step_content = NULL;
static lv_obj_t *g_back_btn = NULL;
static lv_obj_t *g_next_btn = NULL;
static lv_obj_t *g_next_label = NULL;
static lv_obj_t *g_err_label = NULL;

static lv_obj_t *g_spin_normal = NULL;
static lv_obj_t *g_spin_critical = NULL;
static lv_obj_t *g_spin_hyst = NULL;
static lv_obj_t *g_spin_low_adc = NULL;
static lv_obj_t *g_spin_high_adc = NULL;
static lv_obj_t *g_spin_power = NULL;
static lv_obj_t *g_spin_current = NULL;
static lv_obj_t *g_mains_roller = NULL;

static thermal_params_storage_t s_wiz_thermal;
static ato_params_storage_t s_wiz_ato;
static electric_params_storage_t s_wiz_electric;
static system_params_storage_t s_wiz_system;

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

static lv_obj_t *add_spinbox(lv_obj_t *parent, int x, int y, int w, int32_t val, int32_t min, int32_t max)
{
    lv_obj_t *sb = lv_spinbox_create(parent);
    lv_spinbox_set_range(sb, min, max);
    lv_spinbox_set_digit_format(sb, 4, 1);
    lv_spinbox_set_value(sb, val);
    lv_obj_set_size(sb, w, 28);
    lv_obj_set_pos(sb, x, y);
    return sb;
}

static void load_draft_from_config(void)
{
    s_wiz_thermal = *config_get_thermal();
    s_wiz_ato = *config_get_ato();
    s_wiz_electric = *config_get_electric();
    s_wiz_system = *config_get_system();
}

static bool validate_and_save_step(uint8_t step, char *err, size_t err_len)
{
    switch (step) {
        case 2: {
            s_wiz_thermal.temp_normal_c = (float)lv_spinbox_get_value(g_spin_normal) / 10.0f;
            s_wiz_thermal.temp_critical_c = (float)lv_spinbox_get_value(g_spin_critical) / 10.0f;
            s_wiz_thermal.hysteresis_c = (float)lv_spinbox_get_value(g_spin_hyst) / 10.0f;
            if (s_wiz_thermal.temp_min_c >= s_wiz_thermal.temp_normal_c ||
                s_wiz_thermal.temp_normal_c >= s_wiz_thermal.temp_critical_c ||
                s_wiz_thermal.temp_critical_c >= s_wiz_thermal.temp_max_c) {
                snprintf(err, err_len, "Faixas termicas invalidas");
                return false;
            }
            if (config_set_thermal(&s_wiz_thermal) != ESP_OK) {
                snprintf(err, err_len, "Falha ao salvar termico");
                return false;
            }
            break;
        }
        case 3: {
            s_wiz_ato.low_level_adc = lv_spinbox_get_value(g_spin_low_adc);
            s_wiz_ato.high_level_adc = lv_spinbox_get_value(g_spin_high_adc);
            if (s_wiz_ato.low_level_adc >= s_wiz_ato.high_level_adc) {
                snprintf(err, err_len, "ADC ATO invalido");
                return false;
            }
            if (config_set_ato(&s_wiz_ato) != ESP_OK) {
                snprintf(err, err_len, "Falha ao salvar ATO");
                return false;
            }
            break;
        }
        case 4: {
            s_wiz_electric.total_power_limit_w = (float)lv_spinbox_get_value(g_spin_power);
            s_wiz_electric.per_plug_current_limit_a = (float)lv_spinbox_get_value(g_spin_current) / 10.0f;
            uint16_t sel = lv_roller_get_selected(g_mains_roller);
            s_wiz_system.mains_voltage = (sel == 0) ? 127U : 220U;
            if (s_wiz_electric.total_power_limit_w <= 0.0f ||
                s_wiz_electric.per_plug_current_limit_a <= 0.0f) {
                snprintf(err, err_len, "Limites eletricos invalidos");
                return false;
            }
            if (config_set_electric(&s_wiz_electric) != ESP_OK ||
                config_set_system(&s_wiz_system) != ESP_OK) {
                snprintf(err, err_len, "Falha ao salvar eletrica");
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}

static void render_step(uint8_t step)
{
    if (!g_step_desc || !g_step_content) return;

    lv_obj_clean(g_step_content);
    g_spin_normal = g_spin_critical = g_spin_hyst = NULL;
    g_spin_low_adc = g_spin_high_adc = NULL;
    g_spin_power = g_spin_current = NULL;
    g_mains_roller = NULL;
    if (g_err_label) lv_label_set_text(g_err_label, "");

    lv_label_set_text(g_step_title, step_name(step));
    load_draft_from_config();

    switch (step) {
        case 0:
            lv_label_set_text(g_step_desc, "Bem-vindo ao Monitor de Aquario Inteligente.\nConfigure os parametros basicos para comecar.");
            break;
        case 1:
            lv_label_set_text(g_step_desc, "Defina a senha admin via API REST (/api/v1/auth/password).\nAvance apos configurar no painel web.");
            break;
        case 2: {
            lv_label_set_text(g_step_desc, "Ajuste temperatura (x10 = graus C):");
            g_spin_normal = add_spinbox(g_step_content, 10, 10, 100,
                (int32_t)(s_wiz_thermal.temp_normal_c * 10.0f), 180, 350);
            g_spin_critical = add_spinbox(g_step_content, 120, 10, 100,
                (int32_t)(s_wiz_thermal.temp_critical_c * 10.0f), 200, 400);
            g_spin_hyst = add_spinbox(g_step_content, 230, 10, 100,
                (int32_t)(s_wiz_thermal.hysteresis_c * 10.0f), 1, 50);
            lv_obj_t *hint = lv_label_create(g_step_content);
            lv_label_set_text(hint, "Normal | Critica | Histerese");
            lv_obj_set_pos(hint, 10, 42);
            break;
        }
        case 3: {
            lv_label_set_text(g_step_desc, "Limites ADC do sensor ATO:");
            g_spin_low_adc = add_spinbox(g_step_content, 10, 10, 120, s_wiz_ato.low_level_adc, 0, 4095);
            g_spin_high_adc = add_spinbox(g_step_content, 140, 10, 120, s_wiz_ato.high_level_adc, 0, 4095);
            break;
        }
        case 4: {
            lv_label_set_text(g_step_desc, "Limites eletricos e tensao da rede:");
            g_spin_power = add_spinbox(g_step_content, 10, 10, 120,
                (int32_t)s_wiz_electric.total_power_limit_w, 100, 5000);
            g_spin_current = add_spinbox(g_step_content, 140, 10, 120,
                (int32_t)(s_wiz_electric.per_plug_current_limit_a * 10.0f), 10, 200);
            g_mains_roller = lv_roller_create(g_step_content);
            lv_roller_set_options(g_mains_roller, "127V\n220V", LV_ROLLER_MODE_NORMAL);
            lv_roller_set_selected(g_mains_roller, s_wiz_system.mains_voltage == 220 ? 1 : 0, LV_ANIM_OFF);
            lv_obj_set_pos(g_mains_roller, 270, 10);
            lv_obj_set_width(g_mains_roller, 80);
            break;
        }
        case 5: {
            char buf[160];
            snprintf(buf, sizeof(buf),
                     "Revise:\nTemp %.1f/%.1f C | ATO %ld-%ld\nPotencia %.0f W | Rede %u V",
                     (double)s_wiz_thermal.temp_normal_c, (double)s_wiz_thermal.temp_critical_c,
                     (long)s_wiz_ato.low_level_adc, (long)s_wiz_ato.high_level_adc,
                     (double)s_wiz_electric.total_power_limit_w, (unsigned)s_wiz_system.mains_voltage);
            lv_label_set_text(g_step_desc, buf);
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
    g_gs.wizard_step = (wizard_step_t)(step - 1);
    render_step(step - 1);
}

static void wizard_next_cb(lv_event_t *e)
{
    (void)e;
    uint8_t step = config_get_wizard_step();
    char err[64] = {0};

    if (step < WIZARD_STEP_COMPLETE && step >= 2 && step <= 4) {
        if (!validate_and_save_step(step, err, sizeof(err))) {
            if (g_err_label) lv_label_set_text(g_err_label, err);
            return;
        }
        config_save_all();
    }

    if (step >= WIZARD_STEP_COMPLETE) {
        ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
        return;
    }

    if (step == 5) {
        config_set_wizard_completed(true);
        config_set_wizard_step(WIZARD_STEP_COMPLETE);
        g_gs.wizard_completed = true;
        g_gs.wizard_step = WIZARD_STEP_COMPLETE;
        config_save_all();
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

    g_err_label = lv_label_create(parent);
    lv_obj_set_style_text_color(g_err_label, UI_COLOR_CRITICAL, 0);
    lv_obj_set_style_text_font(g_err_label, UI_FONT_SMALL, 0);
    lv_obj_align(g_err_label, LV_ALIGN_TOP_LEFT, 20, 115);

    g_step_content = lv_obj_create(parent);
    lv_obj_set_size(g_step_content, 440, 120);
    lv_obj_set_pos(g_step_content, 20, 130);
    lv_obj_set_style_border_width(g_step_content, 0, 0);
    lv_obj_set_style_bg_opa(g_step_content, LV_OPA_TRANSP, 0);

    g_back_btn = lv_btn_create(parent);
    lv_obj_set_size(g_back_btn, 100, 28);
    lv_obj_align(g_back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -15);
    lv_obj_set_style_bg_color(g_back_btn, UI_COLOR_PANEL_2, 0);
    lv_obj_add_event_cb(g_back_btn, wizard_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(g_back_btn);
    lv_label_set_text(back_lbl, "Voltar");
    lv_obj_center(back_lbl);

    g_next_btn = lv_btn_create(parent);
    lv_obj_set_size(g_next_btn, 100, 28);
    lv_obj_align(g_next_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -15);
    lv_obj_set_style_bg_color(g_next_btn, UI_COLOR_INFO, 0);
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
