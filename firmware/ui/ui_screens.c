// @requirement RF-UI-CAROUSEL-001 Carrossel automático com pausa
// @requirement RF-UI-STATUS-001 Barra de status persistente
// @requirement RF-UI-WIZARD-001..005 Wizard com steps individuais
#include "ui_screens.h"
#include "hardware_config.h"
#include "ui_state_badge.h"
#include "ui_status_bar.h"
#include "global_state.h"
#include "alert_manager.h"
#include "config_manager.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui_screens";

static lv_obj_t *screens[SCREEN_COUNT];
static screen_init_fn_t init_fns[SCREEN_COUNT];
static screen_update_fn_t update_fns[SCREEN_COUNT];
static int current_idx = 0;

static esp_timer_handle_t screen_timeout_timer;
static bool timeout_active = false;

static lv_obj_t *alert_overlay = NULL;
static lv_obj_t *alert_title_label = NULL;
static lv_obj_t *alert_alm_label = NULL;
static lv_obj_t *alert_severity_label = NULL;
static lv_obj_t *alert_action_label = NULL;
static lv_obj_t *alert_ack_label = NULL;

static lv_obj_t *safeoff_overlay = NULL;
static lv_obj_t *safeoff_title = NULL;
static lv_obj_t *safeoff_reason_label = NULL;
static lv_obj_t *safeoff_restart_label = NULL;

static lv_obj_t *emergency_overlay = NULL;
static lv_obj_t *emergency_msg = NULL;

static lv_obj_t *wizard_overlay = NULL;
static lv_obj_t *wizard_msg = NULL;

static lv_obj_t *mute_icon = NULL;
static bool s_muted = false;

// Carousel auto-rotation
static bool s_carousel_enabled = true;
static uint32_t s_carousel_interval_ms = HW_UI_CAROUSEL_INTERVAL_MS;
static uint64_t s_last_carousel_ms = 0;
static uint64_t s_last_interaction_ms = 0;

static const screen_id_t s_carousel_screens[] = {
    SCREEN_DASHBOARD, SCREEN_DEVICES1, SCREEN_DEVICES2,
    SCREEN_ENERGY, SCREEN_DIAGNOSTIC
};
#define CAROUSEL_SCREEN_COUNT (sizeof(s_carousel_screens) / sizeof(s_carousel_screens[0]))

extern global_state_t g_gs;

static void carousel_advance(void)
{
    screen_id_t current = (screen_id_t)current_idx;
    int found = -1;
    for (int i = 0; i < (int)CAROUSEL_SCREEN_COUNT; i++) {
        if (s_carousel_screens[i] == current) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        found = 0;
    } else {
        found = (found + 1) % (int)CAROUSEL_SCREEN_COUNT;
    }
    ui_screen_show(s_carousel_screens[found]);
}

static void on_screen_timeout(void *arg)
{
    ui_screen_show(SCREEN_DASHBOARD);
}

static void restart_timeout(void)
{
    if (timeout_active) {
        esp_timer_stop(screen_timeout_timer);
        esp_timer_start_once(screen_timeout_timer, 60 * 1000 * 1000);
    }
}

void ui_screen_notify_activity(void)
{
    restart_timeout();
    s_last_interaction_ms = esp_timer_get_time() / 1000;
}

void ui_screen_register(screen_id_t id, screen_init_fn_t init, screen_update_fn_t update)
{
    if (id < SCREEN_COUNT) {
        init_fns[id] = init;
        update_fns[id] = update;
    }
}

void ui_toggle_mute(void)
{
    s_muted = !s_muted;
    if (mute_icon) {
        if (s_muted) {
            lv_label_set_text(mute_icon, "[MUTE]");
            lv_obj_clear_flag(mute_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(mute_icon, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

bool ui_is_muted(void)
{
    return s_muted;
}

static lv_obj_t* create_overlay(const char *title, lv_color_t bg, lv_color_t border)
{
    lv_obj_t *ov = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ov, 480, 320);
    lv_obj_center(ov);
    lv_obj_set_style_bg_color(ov, bg, 0);
    lv_obj_set_style_border_color(ov, border, 0);
    lv_obj_set_style_border_width(ov, 3, 0);
    lv_obj_set_style_radius(ov, 0, 0);
    lv_obj_set_style_pad_all(ov, 10, 0);
    lv_obj_add_flag(ov, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ov);

    lv_obj_t *title_lbl = lv_label_create(ov);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 15);

    return ov;
}

esp_err_t ui_screens_init(void)
{
    ESP_LOGI(TAG, "Initializing screens");

    for (int i = 0; i < SCREEN_COUNT; i++) {
        screens[i] = lv_obj_create(NULL);
        lv_obj_set_size(screens[i], 480, 320);
    }

    for (int i = 0; i < SCREEN_COUNT; i++) {
        if (init_fns[i]) {
            init_fns[i](screens[i]);
        }
    }

    ui_status_bar_init();

    esp_timer_create_args_t timeout_args = {
        .callback = on_screen_timeout,
        .name = "screen_timeout"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timeout_args, &screen_timeout_timer));
    timeout_active = true;

    alert_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(alert_overlay, 460, 300);
    lv_obj_center(alert_overlay);
    lv_obj_set_style_bg_color(alert_overlay, lv_color_make(30, 0, 0), 0);
    lv_obj_set_style_border_color(alert_overlay, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_width(alert_overlay, 3, 0);
    lv_obj_set_style_radius(alert_overlay, 8, 0);
    lv_obj_set_style_pad_all(alert_overlay, 10, 0);
    lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(alert_overlay);

    alert_title_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_title_label, "ALERTA CRITICO");
    lv_obj_set_style_text_color(alert_title_label, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_text_font(alert_title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(alert_title_label, LV_ALIGN_TOP_MID, 0, 10);

    alert_alm_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_alm_label, "ALM-000");
    lv_obj_set_style_text_color(alert_alm_label, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_text_font(alert_alm_label, &lv_font_montserrat_16, 0);
    lv_obj_align(alert_alm_label, LV_ALIGN_TOP_LEFT, 10, 45);

    alert_severity_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_severity_label, "Severidade: CRITICAL");
    lv_obj_set_style_text_color(alert_severity_label, lv_color_make(255, 200, 0), 0);
    lv_obj_align(alert_severity_label, LV_ALIGN_TOP_LEFT, 10, 70);

    alert_action_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_action_label, "Acao: Desligar cargas e verificar equipamento");
    lv_obj_set_style_text_color(alert_action_label, lv_color_make(255, 255, 255), 0);
    lv_obj_align(alert_action_label, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_obj_set_size(alert_action_label, 430, 120);
    lv_label_set_long_mode(alert_action_label, LV_LABEL_LONG_WRAP);

    alert_ack_label = lv_label_create(alert_overlay);
    lv_label_set_text(alert_ack_label, "ACK: Enter ou toque para reconhecer");
    lv_obj_set_style_text_color(alert_ack_label, lv_color_make(200, 200, 200), 0);
    lv_obj_align(alert_ack_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    safeoff_overlay = create_overlay("MODO SAFE_OFF", lv_color_make(80, 0, 0), lv_color_make(255, 50, 0));
    safeoff_title = lv_label_create(safeoff_overlay);
    lv_obj_align(safeoff_title, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_text_color(safeoff_title, lv_color_make(255, 200, 0), 0);

    safeoff_reason_label = lv_label_create(safeoff_overlay);
    lv_obj_align(safeoff_reason_label, LV_ALIGN_TOP_LEFT, 20, 90);
    lv_obj_set_style_text_color(safeoff_reason_label, lv_color_make(255, 255, 255), 0);
    lv_obj_set_size(safeoff_reason_label, 440, 100);
    lv_label_set_long_mode(safeoff_reason_label, LV_LABEL_LONG_WRAP);

    safeoff_restart_label = lv_label_create(safeoff_overlay);
    lv_obj_align(safeoff_restart_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(safeoff_restart_label, lv_color_make(200, 200, 200), 0);
    lv_label_set_text(safeoff_restart_label, "Religamento automatico em andamento...");

    emergency_overlay = create_overlay("EMERGENCIA", lv_color_make(120, 0, 0), lv_color_make(255, 0, 0));
    emergency_msg = lv_label_create(emergency_overlay);
    lv_obj_align(emergency_msg, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_color(emergency_msg, lv_color_make(255, 255, 255), 0);
    lv_label_set_text(emergency_msg, "Temperatura EXTREMA detectada!\nDesligamento emergencial ativado.");
    lv_obj_set_style_text_font(emergency_msg, &lv_font_montserrat_16, 0);

    wizard_overlay = create_overlay("WIZARD — CONFIGURACAO INICIAL", lv_color_make(0, 30, 60), lv_color_make(0, 100, 200));
    wizard_msg = lv_label_create(wizard_overlay);
    lv_obj_align(wizard_msg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(wizard_msg, lv_color_make(200, 200, 255), 0);
    lv_label_set_text(wizard_msg, "Bem-vindo ao Monitor de Aquario!\n\nUse a API para configurar:\nPOST /api/v1/wizard\n\n{wizard_step:1} -> Senha admin\n{wizard_step:2} -> Parametros termicos\n{wizard_step:3} -> Configuracao ATO\n{wizard_step:4} -> Parametros eletricos\n{wizard_step:5} -> Revisao final\n{wizard_step:6} -> Concluir");
    lv_obj_set_style_text_font(wizard_msg, &lv_font_montserrat_14, 0);
    lv_obj_set_size(wizard_msg, 440, 240);
    lv_label_set_long_mode(wizard_msg, LV_LABEL_LONG_WRAP);

    mute_icon = lv_label_create(lv_layer_top());
    lv_label_set_text(mute_icon, "[MUTE]");
    lv_obj_set_style_text_color(mute_icon, lv_color_make(255, 200, 0), 0);
    lv_obj_align(mute_icon, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(mute_icon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(mute_icon);

    lv_scr_load(screens[SCREEN_DASHBOARD]);
    restart_timeout();

    ESP_LOGI(TAG, "Screens initialized");
    return ESP_OK;
}

void ui_screen_show(screen_id_t id)
{
    if (id < SCREEN_COUNT && screens[id]) {
        current_idx = id;
        lv_scr_load(screens[id]);
        if (update_fns[id]) {
            update_fns[id]();
        }
        restart_timeout();
    }
}

screen_id_t ui_screen_current(void)
{
    return (screen_id_t)current_idx;
}

void ui_screen_next(void)
{
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        ui_screen_show(SCREEN_ALERTS);
        return;
    }
    current_idx = (current_idx + 1) % SCREEN_COUNT;
    lv_scr_load(screens[current_idx]);
    if (update_fns[current_idx]) {
        update_fns[current_idx]();
    }
    restart_timeout();
}

void ui_screen_prev(void)
{
    if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) {
        ui_screen_show(SCREEN_ALERTS);
        return;
    }
    current_idx = (current_idx - 1 + SCREEN_COUNT) % SCREEN_COUNT;
    lv_scr_load(screens[current_idx]);
    if (update_fns[current_idx]) {
        update_fns[current_idx]();
    }
    restart_timeout();
}

void ui_screen_update_all(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++) {
        if (update_fns[i]) {
            update_fns[i]();
        }
    }

    ui_status_bar_update();

    if (g_gs.system_state == SYSTEM_STATE_EMERGENCY) {
        lv_obj_clear_flag(emergency_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(emergency_overlay);
    } else {
        lv_obj_add_flag(emergency_overlay, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_gs.system_state == SYSTEM_STATE_SAFE_OFF) {
        lv_obj_clear_flag(safeoff_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(safeoff_overlay);

        char buf[128];
        snprintf(buf, sizeof(buf), "Causa: %s", g_gs.safeoff_source_alm);
        lv_label_set_text(safeoff_title, buf);

        const char *reason_str = "Motivo desconhecido";
        switch (g_gs.safeoff_reason) {
            case SAFEOFF_REASON_THERMAL_CRITICAL: reason_str = "Temperatura critica atingida"; break;
            case SAFEOFF_REASON_THERMAL_EXTREME: reason_str = "Temperatura extrema! Emergencia!"; break;
            case SAFEOFF_REASON_ATO_OVERFLOW: reason_str = "Overflow do reservatorio ATO"; break;
            case SAFEOFF_REASON_ELECTRIC_TOTAL: reason_str = "Sobrecarga eletrica total"; break;
            case SAFEOFF_REASON_PLUG_SHORT: reason_str = "Curto-circuito detectado"; break;
            case SAFEOFF_REASON_SELFTEST_FAIL: reason_str = "Self-test de inicializacao falhou"; break;
            case SAFEOFF_REASON_MCP23017_FAIL: reason_str = "Falha no expansor MCP23017"; break;
            default: break;
        }
        lv_label_set_text(safeoff_reason_label, reason_str);

        if (g_gs.restart_in_progress) {
            lv_label_set_text(safeoff_restart_label, "Religamento automatico em andamento...");
            lv_obj_clear_flag(safeoff_restart_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(safeoff_restart_label, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(safeoff_overlay, LV_OBJ_FLAG_HIDDEN);
    }

    if (!g_gs.wizard_completed) {
        lv_obj_clear_flag(wizard_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(wizard_overlay);

        char wbuf[320];
        const char *step_text = "Bem-vindo!\n\nUse a API REST para configurar:\nPOST /api/v1/wizard";
        switch (g_gs.wizard_step) {
            case 0: step_text = "Bem-vindo ao Monitor de Aquario!\n\nUse a API para comecar:\nPOST /api/v1/wizard com wizard_step=1"; break;
            case 1: step_text = "Passo 1/6: Senha Admin\n\nDefina a senha de administrador:\n{\"password\":\"sua_senha\",\"wizard_step\":2}"; break;
            case 2: step_text = "Passo 2/6: Parametros Termicos\n\nConfigure temperatura normal, critica e histerese:\n{\"thermal\":{...},\"wizard_step\":3}"; break;
            case 3: step_text = "Passo 3/6: Configuracao ATO\n\nNiveis ADC baixo/alto e timeout de refill:\n{\"ato\":{...},\"wizard_step\":4}"; break;
            case 4: step_text = "Passo 4/6: Parametros Eletricos\n\nLimites de potencia, tensao e corrente:\n{\"electric\":{...},\"wizard_step\":5}"; break;
            case 5: step_text = "Passo 5/6: Revisao Final\n\nConfirme os parametros:\n{\"wizard_completed\":true}"; break;
            default: break;
        }
        snprintf(wbuf, sizeof(wbuf), "%s\n\nStep: %d/6", step_text, g_gs.wizard_step);
        lv_label_set_text(wizard_msg, wbuf);
    } else {
        lv_obj_add_flag(wizard_overlay, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_gs.critical_alerts_count > 0 && g_gs.system_state < SYSTEM_STATE_SAFE_OFF) {
        lv_obj_clear_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(alert_overlay);

        char buf[64];
        snprintf(buf, sizeof(buf), "ALM-XXX | %d alerta(s) critico(s)", g_gs.critical_alerts_count);
        lv_label_set_text(alert_alm_label, buf);

        const char *action = "Desligar cargas e verificar equipamento";
        if (g_gs.safeoff_reason == SAFEOFF_REASON_THERMAL_CRITICAL) {
            action = "Temperatura critica! Verificar aquecedor e cooler";
        } else if (g_gs.safeoff_reason == SAFEOFF_REASON_ATO_OVERFLOW) {
            action = "Nivel ATO em overflow! Verificar reservatorio";
        } else if (g_gs.safeoff_reason == SAFEOFF_REASON_ELECTRIC_TOTAL) {
            action = "Sobrecarga eletrica! Reduzir consumo";
        } else if (g_gs.safeoff_reason == SAFEOFF_REASON_PLUG_SHORT) {
            action = "Curto-circuito detectado! Desconectar equipamento";
        }
        lv_label_set_text(alert_action_label, action);
    } else {
        lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
    }

    if (s_carousel_enabled) {
        if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) return;
        if (!g_gs.wizard_completed) return;
        uint64_t now_ms = esp_timer_get_time() / 1000;
        if (now_ms - s_last_interaction_ms < HW_UI_CAROUSEL_PAUSE_ON_ACTIVITY_MS) return;
        if (now_ms - s_last_carousel_ms > s_carousel_interval_ms) {
            carousel_advance();
            s_last_carousel_ms = now_ms;
        }
    }
}

void ui_carousel_enable(bool en)
{
    s_carousel_enabled = en;
}

void ui_carousel_set_interval(uint32_t interval_ms)
{
    s_carousel_interval_ms = interval_ms;
}
