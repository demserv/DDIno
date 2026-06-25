// @requirement RF-UI-DIAG-001 Tela de diagnóstico
#include "../ui_screens.h"
#include "global_state.h"
#include "system_types.h"
#include "plug_model.h"
#include "services/plug_manager.h"
#include "driver_pzem.h"
#include "driver_ds18b20.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

static const char *TAG = "screen_diagnostic";

#define DIAG_LABEL_COUNT 14

static lv_obj_t *diag_cont = NULL;
static lv_obj_t *diag_labels[DIAG_LABEL_COUNT];

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

static float prev_filtered = -100.0f;

static const char *state_str(system_state_t s)
{
    switch (s) {
        case SYSTEM_STATE_NORMAL:   return "NORMAL";
        case SYSTEM_STATE_DEGRADED: return "DEGRADADO";
        case SYSTEM_STATE_SAFE_OFF: return "SAFE_OFF";
        case SYSTEM_STATE_EMERGENCY:return "EMERGENCIA";
        default:                    return "DESCONHECIDO";
    }
}

static const char *safeoff_reason_str(safeoff_reason_t r)
{
    switch (r) {
        case SAFEOFF_REASON_NONE:              return "Nenhum";
        case SAFEOFF_REASON_THERMAL_CRITICAL:  return "Temp. Critica";
        case SAFEOFF_REASON_THERMAL_EXTREME:   return "Temp. Extrema";
        case SAFEOFF_REASON_ATO_OVERFLOW:      return "Overflow ATO";
        case SAFEOFF_REASON_ELECTRIC_TOTAL:    return "Sobrecarga Eletrica";
        case SAFEOFF_REASON_PLUG_SHORT:        return "Curto-circuito";
        case SAFEOFF_REASON_SELFTEST_FAIL:     return "Self-test falhou";
        case SAFEOFF_REASON_MCP23017_FAIL:     return "Falha MCP23017";
        case SAFEOFF_REASON_CONFIG_INVALID:    return "Config. Invalida";
        case SAFEOFF_REASON_FSM_INVALID:       return "FSM Invalido";
        case SAFEOFF_REASON_WDT_RECOVERY:      return "WDT Recovery";
        case SAFEOFF_REASON_MANUAL_CRITICAL:   return "Manual Critico";
        case SAFEOFF_REASON_OVERVOLTAGE:       return "Sobretensao";
        case SAFEOFF_REASON_UNDERVOLTAGE:      return "Subtensao";
        default:                               return "Outro";
    }
}

static const char *reset_reason_str(uint32_t reason)
{
    switch (reason) {
        case 0:  return "DESCONHECIDO";
        case 1:  return "POWER_ON";
        case 2:  return "EXT_PIN";
        case 3:  return "SW_RESET";
        case 4:  return "PANIC";
        case 5:  return "INT_WDT";
        case 6:  return "TASK_WDT";
        case 7:  return "WDT";
        case 8:  return "DEEP_SLEEP";
        case 9:  return "BROWNOUT";
        case 10: return "SDIO";
        default: return "OUTRO";
    }
}

static const char *trend_char(float current, float previous)
{
    if (previous < -50.0f) return " -";
    float diff = current - previous;
    if (diff > 0.3f) return " \xe2\x86\x91";
    if (diff < -0.3f) return " \xe2\x86\x93";
    return " \xe2\x86\x92";
}

static void screen_init_diagnostic(lv_obj_t *parent)
{
    diag_cont = lv_obj_create(parent);
    lv_obj_set_size(diag_cont, 480, 290);
    lv_obj_align(diag_cont, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_border_width(diag_cont, 0, 0);
    lv_obj_set_style_pad_all(diag_cont, 5, 0);
    lv_obj_clear_flag(diag_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = lv_label_create(diag_cont);
    lv_label_set_text(header, "=== DIAGNOSTICO ===");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

    for (int i = 0; i < DIAG_LABEL_COUNT; i++) {
        diag_labels[i] = lv_label_create(diag_cont);
        lv_label_set_text(diag_labels[i], "--");
        lv_obj_align(diag_labels[i], LV_ALIGN_TOP_LEFT, 10, 25 + i * 19);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< >   Navegar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_diagnostic(void)
{
    char buf[96];
    int idx = 0;

    snprintf(buf, sizeof(buf), "Estado: %s", state_str(g_gs.system_state));
    lv_label_set_text(diag_labels[idx++], buf);

    float raw_temp = 25.0f;
    float filtered = g_gs.temp_filtered_c;
    ds18b20_read(&raw_temp);
    const char *trend = trend_char(filtered, prev_filtered);
    prev_filtered = filtered;
    snprintf(buf, sizeof(buf), "Temp: %.1f C | Filt: %.1f C |%s",
             (double)raw_temp, (double)filtered, trend);
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "ATO: %s", g_gs.ato_ok ? "OK" : "FALHA");
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "Rede: %.1fV %.2fA %.0fW %.1fHz",
             (double)g_pzem.voltage_v, (double)g_pzem.current_a,
             (double)g_pzem.power_w, (double)g_pzem.frequency_hz);
    lv_label_set_text(diag_labels[idx++], buf);

    uint8_t plugs_on = plug_manager_active_count();
    snprintf(buf, sizeof(buf), "Tomadas: %u de 10 ligadas", plugs_on);
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "SD: %s | WiFi: %s",
             g_gs.sd_ok ? "OK" : "N/A",
             g_gs.wifi_ok ? "Conectado" : "Desconectado");
    lv_label_set_text(diag_labels[idx++], buf);

    uint64_t uptime = g_gs.uptime_s;
    uint64_t h = uptime / 3600;
    uint64_t m = (uptime % 3600) / 60;
    uint64_t sec = uptime % 60;
    snprintf(buf, sizeof(buf), "Uptime: %lluh %02llum %02llus",
             (unsigned long long)h, (unsigned long long)m, (unsigned long long)sec);
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "Alarmes: %u crit / %u ativos",
             g_gs.critical_alerts_count, g_gs.active_alerts_count);
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "WDT Resets: 0");
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "Ultimo Reset: %s", reset_reason_str(g_gs.last_reset_reason));
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "Safeoff: %s", safeoff_reason_str(g_gs.safeoff_reason));
    lv_label_set_text(diag_labels[idx++], buf);

    uint32_t free_heap = esp_get_free_heap_size() / 1024;
    snprintf(buf, sizeof(buf), "Heap: %lu KB", (unsigned long)free_heap);
    lv_label_set_text(diag_labels[idx++], buf);

    snprintf(buf, sizeof(buf), "FW: %s", g_gs.fw_version);
    lv_label_set_text(diag_labels[idx++], buf);
}

static void __attribute__((constructor)) register_diagnostic(void)
{
    ui_screen_register(SCREEN_DIAGNOSTIC, screen_init_diagnostic, screen_update_diagnostic);
}
