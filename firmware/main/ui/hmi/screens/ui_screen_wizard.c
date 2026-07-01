// @requirement RF-UI-WIZARD-001..005 Wizard inicial obrigatório (16 etapas, SRS §72)
// @requirement RF-UI-WIZARD-004 Ordem oficial mínima do Wizard
// @requirement RF-UI-WIZARD-005 Reentrada segura (passo persistido em NVS)
// @requirement RF-UI-WIZARD-00X.1 Validação antes de avançar + feedback
// @requirement RF-UI-WIZARD-DEFAULT-REVIEW-001 Revisão obrigatória dos defaults críticos
#include "ui_screen_wizard.h"
#include "../ui_theme.h"
#include "../ui_screen_manager.h"
#include "global_state.h"
#include "config_manager.h"
#include "param_catalog.h"
#include "hardware_config.h"
#include "storage_sd.h"
#include "wifi_ctl.h"
#include "esp_log.h"
#include <stdint.h>
#include <string.h>

static const char *TAG = "wizard";

extern global_state_t g_gs;

static lv_obj_t *g_step_title = NULL;
static lv_obj_t *g_step_desc = NULL;
static lv_obj_t *g_step_content = NULL;
static lv_obj_t *g_step_error = NULL;
static lv_obj_t *g_back_btn = NULL;
static lv_obj_t *g_next_btn = NULL;
static lv_obj_t *g_next_label = NULL;

static void render_step(uint8_t step);

static const char *step_name(uint8_t step)
{
    switch (step) {
        case WIZARD_STEP_WELCOME:     return "Boas-vindas";
        case WIZARD_STEP_HW_CHECK:    return "Hardware";
        case WIZARD_STEP_UI_ADJUST:   return "Ajuste de UI";
        case WIZARD_STEP_KEYPAD:      return "Calibracao Keypad";
        case WIZARD_STEP_THERMAL:     return "Temperatura";
        case WIZARD_STEP_ATO:         return "ATO";
        case WIZARD_STEP_PLUGS:       return "Plugues Criticos";
        case WIZARD_STEP_ELECTRIC:    return "Protecao Eletrica";
        case WIZARD_STEP_RESILIENCE:  return "Resiliencia";
        case WIZARD_STEP_SECURITY:    return "Seguranca";
        case WIZARD_STEP_MAINTENANCE: return "Manutencao";
        case WIZARD_STEP_PROFILES:    return "Perfis/Config";
        case WIZARD_STEP_STORAGE:     return "Validacao Storage";
        case WIZARD_STEP_REVIEW:      return "Resumo Final";
        case WIZARD_STEP_CONFIRM:     return "Confirmacao";
        default:                      return "Concluido";
    }
}

/* @requirement RF-UI-WIZARD-00X.1 Validação obrigatória de campos críticos antes do avanço. */
static bool step_is_valid(uint8_t step, const char **err)
{
    *err = NULL;
    switch (step) {
        case WIZARD_STEP_THERMAL: {
            const thermal_params_storage_t *t = config_get_thermal();
            if (!(t->temp_critical_c > t->temp_normal_c)) { *err = "Critica deve ser > Normal"; return false; }
            if (t->extreme_enabled && !(t->temp_extreme_c > t->temp_critical_c)) { *err = "Extrema deve ser > Critica"; return false; }
            if (!(t->hysteresis_c > 0.0f)) { *err = "Histerese deve ser > 0"; return false; }
            return true;
        }
        case WIZARD_STEP_ATO: {
            const ato_params_storage_t *a = config_get_ato();
            if (a->enabled && !(a->high_level_adc > a->low_level_adc)) { *err = "Nivel ALTO deve ser > BAIXO"; return false; }
            if (a->enabled && a->refill_timeout_s == 0) { *err = "Timeout de refill deve ser > 0"; return false; }
            return true;
        }
        case WIZARD_STEP_ELECTRIC: {
            const system_params_storage_t *s = config_get_system();
            const electric_params_storage_t *e = config_get_electric();
            if (s->mains_voltage != 127 && s->mains_voltage != 220) {
                *err = "Selecione a tensao nominal (127 ou 220 V)"; return false;
            }
            if (!(e->undervoltage_limit_v > 0.0f) || !(e->overvoltage_limit_v > e->undervoltage_limit_v)) {
                *err = "Limites de tensao invalidos"; return false;
            }
            if (!(e->per_plug_current_limit_a > 0.0f) || !(e->total_power_limit_w > 0.0f)) {
                *err = "Limites eletricos invalidos"; return false;
            }
            return true;
        }
        default:
            return true;
    }
}

/* @requirement RF-UI-WIZARD-002 Seleção obrigatória de tensão nominal 127/220 V (rede bivolt).
 * Ajusta os limites de sobre/subtensão para o perfil selecionado e persiste em NVS. */
static void volt_sel_cb(lv_event_t *e)
{
    uint16_t v = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    system_params_storage_t sys = *config_get_system();
    electric_params_storage_t el = *config_get_electric();
    sys.mains_voltage = v;
    if (v == 220) {
        el.overvoltage_limit_v  = PARAM_ELECTRIC_DEFAULT_OVERVOLTAGE_220V;
        el.undervoltage_limit_v = PARAM_ELECTRIC_DEFAULT_UNDERVOLTAGE_220V;
    } else {
        el.overvoltage_limit_v  = PARAM_ELECTRIC_DEFAULT_OVERVOLTAGE_127V;
        el.undervoltage_limit_v = PARAM_ELECTRIC_DEFAULT_UNDERVOLTAGE_127V;
    }
    config_set_system(&sys);
    config_set_electric(&el);
    ESP_LOGI(TAG, "Tensao nominal selecionada: %u V", (unsigned)v);
    render_step(WIZARD_STEP_ELECTRIC);
}

static void make_volt_button(lv_obj_t *parent, const char *txt, uint16_t v, bool selected, lv_coord_t x)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, 120, 40);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, x, 96);
    lv_obj_set_style_bg_color(b, selected ? UI_COLOR_INFO : UI_COLOR_PANEL_2, 0);
    lv_obj_set_style_radius(b, UI_RADIUS_BUTTON, 0);
    lv_obj_add_event_cb(b, volt_sel_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)v);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
}

static void render_step(uint8_t step)
{
    if (!g_step_desc || !g_step_content) return;

    lv_obj_clean(g_step_content);
    if (g_step_error) lv_label_set_text(g_step_error, "");

    char title_buf[56];
    snprintf(title_buf, sizeof(title_buf), "Passo %u/%d: %s",
             (unsigned)(step + 1), WIZARD_TOTAL_STEPS, step_name(step));
    lv_label_set_text(g_step_title, title_buf);

    char buf[320];

    switch (step) {
        case WIZARD_STEP_WELCOME:
            lv_label_set_text(g_step_desc,
                "Bem-vindo ao Monitor de Aquario Inteligente.\n"
                "Este assistente revisa os parametros criticos\n"
                "antes de liberar as automacoes. Avance para revisar\n"
                "cada grupo. Nada e liberado ate a confirmacao final.");
            break;

        case WIZARD_STEP_HW_CHECK:
            snprintf(buf, sizeof(buf),
                     "Hardware detectado:\n"
                     "Temp (DS18B20): %s\nNivel ATO: %s\nEnergia (PZEM): %s\n"
                     "Cartao SD: %s\nUI/Display: %s\nWiFi: %s",
                     g_gs.temp_ok ? "OK" : "--",
                     g_gs.ato_ok ? "OK" : "--",
                     g_gs.pzem_ok ? "OK" : "--",
                     g_gs.sd_ok ? "OK" : "--",
                     g_gs.ui_ok ? "OK" : "--",
                     g_gs.wifi_ok ? "OK" : "--");
            lv_label_set_text(g_step_desc, buf);
            break;

        case WIZARD_STEP_UI_ADJUST:
            snprintf(buf, sizeof(buf),
                     "Ajuste basico de UI:\nBrilho padrao: %u%%\nContraste padrao: %u\n"
                     "Ajustes finos disponiveis no menu Configuracoes.",
                     (unsigned)HW_DISP_BRIGHTNESS_DEFAULT, (unsigned)HW_DISP_CONTRAST_DEFAULT);
            lv_label_set_text(g_step_desc, buf);
            break;

        case WIZARD_STEP_KEYPAD:
            lv_label_set_text(g_step_desc,
                "Calibracao do keypad AD (MCP3208 CH3):\n"
                "Os limiares de tecla usam a calibracao de fabrica.\n"
                "A recalibracao assistida esta disponivel no menu\n"
                "Manutencao quando o hardware estiver montado.");
            break;

        case WIZARD_STEP_THERMAL: {
            const thermal_params_storage_t *tp = config_get_thermal();
            snprintf(buf, sizeof(buf),
                     "Temperatura:\nNormal: %.1f C\nCritica: %.1f C\nExtrema: %.1f C\nHisterese: %.1f C\nExtrema ativa: %s",
                     (double)tp->temp_normal_c, (double)tp->temp_critical_c,
                     (double)tp->temp_extreme_c, (double)tp->hysteresis_c,
                     tp->extreme_enabled ? "Sim" : "Nao");
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_ATO: {
            const ato_params_storage_t *ap = config_get_ato();
            snprintf(buf, sizeof(buf),
                     "ATO (%s):\nNivel baixo ADC: %ld\nNivel alto ADC: %ld\nMargem overflow: %ld\nTimeout: %lu s",
                     ap->enabled ? "habilitado" : "desabilitado",
                     (long)ap->low_level_adc, (long)ap->high_level_adc,
                     (long)ap->overflow_margin_adc, (unsigned long)ap->refill_timeout_s);
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_PLUGS: {
            const plug_limits_storage_t *pl = config_get_plug_limits();
            snprintf(buf, sizeof(buf),
                     "Plugues criticos (P01/P02):\nTempo min ON: %lu s\nTempo min OFF: %lu s\n"
                     "Defina cargas criticas (aquecedor, bomba ATO)\nno menu Configuracoes > Plugues.",
                     (unsigned long)pl->min_on_time_s, (unsigned long)pl->min_off_time_s);
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_ELECTRIC: {
            const electric_params_storage_t *ep = config_get_electric();
            const system_params_storage_t *sp = config_get_system();
            bool sel = (sp->mains_voltage == 127 || sp->mains_voltage == 220);
            snprintf(buf, sizeof(buf),
                     "Protecao Eletrica (rede bivolt):\nTensao nominal: %s\n"
                     "Sobretensao: %.0f V  Subtensao: %.0f V\nPotencia: %.0f W  Corrente/tomada: %.1f A\n"
                     "Selecione a tensao nominal:",
                     sel ? (sp->mains_voltage == 220 ? "220 V" : "127 V") : "NAO SELECIONADA",
                     (double)ep->overvoltage_limit_v, (double)ep->undervoltage_limit_v,
                     (double)ep->total_power_limit_w, (double)ep->per_plug_current_limit_a);
            lv_label_set_text(g_step_desc, buf);
            make_volt_button(g_step_content, "127 V", 127, sp->mains_voltage == 127, 0);
            make_volt_button(g_step_content, "220 V", 220, sp->mains_voltage == 220, 140);
            break;
        }

        case WIZARD_STEP_RESILIENCE: {
            const restart_params_storage_t *rp = config_get_restart();
            const antiflap_params_storage_t *af = config_get_antiflap();
            snprintf(buf, sizeof(buf),
                     "Resiliencia (religamento/anti-flapping):\n"
                     "Espera religamento: %lu s\nIntervalo: %lu s\nMonitoramento pos: %lu s\n"
                     "Janela flap: %lu s / max %lu transicoes\nCooldown reentrada: %lu s",
                     (unsigned long)rp->tempo_espera_religamento_s,
                     (unsigned long)rp->intervalo_religamento_s,
                     (unsigned long)rp->tempo_monitoramento_pos_relig_s,
                     (unsigned long)af->janela_flap_s,
                     (unsigned long)af->max_transicoes_flap,
                     (unsigned long)af->cooldown_reentrada_s);
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_SECURITY: {
            const security_params_storage_t *sec = config_get_security();
            snprintf(buf, sizeof(buf),
                     "Seguranca Web:\nTimeout de sessao: %lu min\nMax tentativas login: %lu\n"
                     "Bloqueio por IP: %lu min\nTimeout ACK: %lu s\nLeitura exige auth: %s\n"
                     "Senha admin definida via API/menu.",
                     (unsigned long)sec->session_timeout_min,
                     (unsigned long)sec->max_login_attempts,
                     (unsigned long)sec->login_block_duration_min,
                     (unsigned long)sec->ack_timeout_s,
                     sec->read_requires_auth ? "Sim" : "Nao");
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_MAINTENANCE: {
            const system_params_storage_t *sp = config_get_system();
            snprintf(buf, sizeof(buf),
                     "Manutencao:\nModo manutencao: %s\nMonitor Only Mode: %s\n"
                     "Nestes modos as automacoes ficam suspensas\ne apenas o monitoramento permanece ativo.",
                     sp->maintenance_mode ? "Ativo" : "Inativo",
                     sp->monitor_only_mode ? "Ativo" : "Inativo");
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_PROFILES: {
            const system_params_storage_t *sp = config_get_system();
            snprintf(buf, sizeof(buf),
                     "Perfis / Config final:\nTensao nominal: %u V\nMonitor Only: %s\n"
                     "Perfis de plugue podem ser aplicados no menu\nConfiguracoes > Perfis apos a conclusao.",
                     (unsigned)sp->mains_voltage,
                     sp->monitor_only_mode ? "Sim" : "Nao");
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_STORAGE: {
            bool sd = storage_sd_is_mounted();
            uint64_t total = 0, freeb = 0;
            const char *space = "";
            char sbuf[48];
            if (sd && storage_sd_get_space(&total, &freeb) == ESP_OK && total > 0) {
                snprintf(sbuf, sizeof(sbuf), "\nLivre: %llu / %llu MB",
                         (unsigned long long)(freeb / (1024ULL * 1024ULL)),
                         (unsigned long long)(total / (1024ULL * 1024ULL)));
                space = sbuf;
            }
            snprintf(buf, sizeof(buf),
                     "Validacao de storage:\nCartao SD: %s%s\n"
                     "Logs e backup dependem do SD. A configuracao\ne persistida em NVS mesmo sem SD.",
                     sd ? "Montado" : "Indisponivel", space);
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_REVIEW: {
            const thermal_params_storage_t *tp = config_get_thermal();
            const electric_params_storage_t *ep = config_get_electric();
            const system_params_storage_t *sp = config_get_system();
            const ato_params_storage_t *ap = config_get_ato();
            snprintf(buf, sizeof(buf),
                     "Resumo (revise os grupos criticos):\n"
                     "Termica: N%.0f/C%.0f/E%.0f C  ATO:%s\n"
                     "Eletrica: %u V  OV%.0f/UV%.0f V  %.1f A/tomada\n"
                     "Seguranca/Manut/Storage revisados.\n"
                     "Confirme na proxima etapa para gravar.",
                     (double)tp->temp_normal_c, (double)tp->temp_critical_c, (double)tp->temp_extreme_c,
                     ap->enabled ? "on" : "off",
                     (unsigned)sp->mains_voltage,
                     (double)ep->overvoltage_limit_v, (double)ep->undervoltage_limit_v,
                     (double)ep->per_plug_current_limit_a);
            lv_label_set_text(g_step_desc, buf);
            break;
        }

        case WIZARD_STEP_CONFIRM:
            lv_label_set_text(g_step_desc,
                "Confirmacao e gravacao:\n"
                "Ao concluir, a configuracao sera persistida e as\n"
                "automacoes serao liberadas. Pressione 'Concluir'\n"
                "para gravar e entrar na Home.");
            break;

        default:
            lv_label_set_text(g_step_desc, "Wizard concluido!");
            break;
    }
}

static void set_next_label_for(uint8_t step)
{
    if (!g_next_label) return;
    lv_label_set_text(g_next_label, (step == WIZARD_STEP_CONFIRM) ? "Concluir" : "Avancar");
}

static void wizard_back_cb(lv_event_t *e)
{
    (void)e;
    uint8_t step = config_get_wizard_step();
    if (step == WIZARD_STEP_WELCOME || step >= WIZARD_STEP_COMPLETE) {
        ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
        return;
    }
    uint8_t prev = step - 1;
    config_set_wizard_step(prev);
    g_gs.wizard_step = (wizard_step_t)prev;
    render_step(prev);
    set_next_label_for(prev);
}

static void wizard_next_cb(lv_event_t *e)
{
    (void)e;
    uint8_t step = config_get_wizard_step();
    if (step >= WIZARD_STEP_COMPLETE) {
        ui_screen_manager_show(UI_SCREEN_MAIN_MENU);
        return;
    }

    /* @requirement RF-UI-WIZARD-00X.1 Bloqueia avanço com campos inválidos. */
    const char *err = NULL;
    if (!step_is_valid(step, &err)) {
        if (g_step_error) lv_label_set_text(g_step_error, err ? err : "Campo invalido");
        ESP_LOGW(TAG, "Passo %u invalido: %s", (unsigned)step, err ? err : "?");
        return;
    }

    /* @requirement RF-UI-WIZARD-004 Confirmação/gravação apenas na última etapa. */
    if (step == WIZARD_STEP_CONFIRM) {
        config_set_wizard_completed(true);
        config_set_wizard_step(WIZARD_STEP_COMPLETE);
        g_gs.wizard_completed = true;
        g_gs.wizard_step = WIZARD_STEP_COMPLETE;
        ESP_LOGI(TAG, "Wizard concluido (16 etapas), sistema liberado");
        ui_screen_manager_show(UI_SCREEN_DASHBOARD);
        return;
    }

    uint8_t next = step + 1;
    config_set_wizard_step(next);
    g_gs.wizard_step = (wizard_step_t)next;
    render_step(next);
    set_next_label_for(next);
}

void ui_screen_wizard_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Wizard Inicial");
    lv_obj_set_style_text_font(title, UI_FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    g_step_title = lv_label_create(parent);
    lv_obj_set_style_text_font(g_step_title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(g_step_title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(g_step_title, LV_ALIGN_TOP_LEFT, 20, 40);

    g_step_desc = lv_label_create(parent);
    lv_obj_set_width(g_step_desc, 440);
    lv_obj_set_style_text_font(g_step_desc, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(g_step_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(g_step_desc, LV_ALIGN_TOP_LEFT, 20, 68);

    g_step_content = lv_obj_create(parent);
    lv_obj_set_size(g_step_content, 440, 150);
    lv_obj_set_pos(g_step_content, 20, 150);
    lv_obj_set_style_border_width(g_step_content, 0, 0);
    lv_obj_set_style_bg_opa(g_step_content, LV_OPA_TRANSP, 0);

    g_step_error = lv_label_create(parent);
    lv_obj_set_width(g_step_error, 440);
    lv_obj_set_style_text_font(g_step_error, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(g_step_error, UI_COLOR_CRITICAL, 0);
    lv_obj_align(g_step_error, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_label_set_text(g_step_error, "");

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
    lv_obj_center(g_next_label);

    uint8_t step = config_get_wizard_step();
    if (step >= WIZARD_STEP_COMPLETE) step = WIZARD_STEP_WELCOME;
    set_next_label_for(step);
    render_step(step);
}

void ui_screen_wizard_update(ui_root_vm_t *vm)
{
    (void)vm;
    render_step(config_get_wizard_step());
}
