#include "../ui_screens.h"
#include "global_state.h"
#include "plug_model.h"
#include "services/plug_manager.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "screen_diagnostic";

static lv_obj_t *diag_labels[12];

extern global_state_t g_gs;

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
        case SAFEOFF_REASON_ATO_OVERFLOW:      return "Overflow ATO";
        case SAFEOFF_REASON_ELECTRIC_TOTAL:    return "Sobrecarga";
        case SAFEOFF_REASON_PLUG_SHORT:        return "Curto-circuito";
        case SAFEOFF_REASON_SELFTEST_FAIL:     return "Self-test falhou";
        default:                               return "Outro";
    }
}

static void screen_init_diagnostic(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Diagnostico");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);

    const int col1_x = 10;
    const int col2_x = 240;
    const int row_h = 23;

    for (int i = 0; i < 12; i++) {
        int col = (i < 6) ? col1_x : col2_x;
        int row = (i % 6);
        diag_labels[i] = lv_label_create(parent);
        lv_label_set_text(diag_labels[i], "--");
        lv_obj_align(diag_labels[i], LV_ALIGN_TOP_LEFT, col, 35 + row * row_h);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< >   Navegar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_diagnostic(void)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "Estado: %s", state_str(g_gs.system_state));
    lv_label_set_text(diag_labels[0], buf);

    snprintf(buf, sizeof(buf), "Safe-off: %s", safeoff_reason_str(g_gs.safeoff_reason));
    lv_label_set_text(diag_labels[1], buf);

    snprintf(buf, sizeof(buf), "Temp: %s", g_gs.temp_ok ? "OK" : "FALHA");
    lv_label_set_text(diag_labels[2], buf);

    snprintf(buf, sizeof(buf), "PZEM: %s", g_gs.pzem_ok ? "OK" : "FALHA");
    lv_label_set_text(diag_labels[3], buf);

    snprintf(buf, sizeof(buf), "ATO: %s", g_gs.ato_ok ? "OK" : "FALHA");
    lv_label_set_text(diag_labels[4], buf);

    snprintf(buf, sizeof(buf), "SD: %s", g_gs.sd_ok ? "OK" : "N/A");
    lv_label_set_text(diag_labels[5], buf);

    snprintf(buf, sizeof(buf), "WiFi: %s", g_gs.wifi_ok ? "Conectado" : "Desconectado");
    lv_label_set_text(diag_labels[6], buf);

    snprintf(buf, sizeof(buf), "Uptime: %llus", (unsigned long long)g_gs.uptime_s);
    lv_label_set_text(diag_labels[7], buf);

    uint32_t free_heap = esp_get_free_heap_size() / 1024;
    snprintf(buf, sizeof(buf), "Heap: %lu KB", (unsigned long)free_heap);
    lv_label_set_text(diag_labels[8], buf);

    snprintf(buf, sizeof(buf), "Alarmes: %d crit / %d total",
             g_gs.critical_alerts_count, g_gs.active_alerts_count);
    lv_label_set_text(diag_labels[9], buf);

    snprintf(buf, sizeof(buf), "FW: %s", g_gs.fw_version);
    lv_label_set_text(diag_labels[10], buf);

    uint8_t plugs_on = plug_manager_active_count();
    snprintf(buf, sizeof(buf), "Tomadas: %d/10 ligadas", plugs_on);
    lv_label_set_text(diag_labels[11], buf);
}

static void __attribute__((constructor)) register_diagnostic(void)
{
    ui_screen_register(SCREEN_DIAGNOSTIC, screen_init_diagnostic, screen_update_diagnostic);
}
