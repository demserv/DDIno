#include "../ui_screens.h"
#include "../ui_state_badge.h"
#include "global_state.h"
#include "plug_model.h"
#include "alert_model.h"
#include "driver_pzem.h"
#include "driver_ds18b20.h"

#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "screen_dashboard";

static lv_obj_t *temp_label = NULL;
static lv_obj_t *plugs_label = NULL;
static lv_obj_t *alerts_label = NULL;
static lv_obj_t *energy_label = NULL;

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

static void screen_init_dashboard(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "DDIno - Monitor de Aquario");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *left_col = lv_obj_create(parent);
    lv_obj_set_size(left_col, 220, 200);
    lv_obj_set_style_border_width(left_col, 0, 0);
    lv_obj_align(left_col, LV_ALIGN_LEFT_MID, 10, 10);
    lv_obj_clear_flag(left_col, LV_OBJ_FLAG_SCROLLABLE);

    temp_label = lv_label_create(left_col);
    lv_label_set_text(temp_label, "Temperatura: --.- C");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_16, 0);
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 0, 0);

    plugs_label = lv_label_create(left_col);
    lv_label_set_text(plugs_label, "Tomadas: 0");
    lv_obj_align(plugs_label, LV_ALIGN_TOP_LEFT, 0, 40);

    alerts_label = lv_label_create(left_col);
    lv_label_set_text(alerts_label, "Alarmes: 0");
    lv_obj_align(alerts_label, LV_ALIGN_TOP_LEFT, 0, 70);

    energy_label = lv_label_create(left_col);
    lv_label_set_text(energy_label, "Energia: --.- V / --- W");
    lv_obj_align(energy_label, LV_ALIGN_TOP_LEFT, 0, 100);

    lv_obj_t *bottom = lv_label_create(parent);
    lv_label_set_text(bottom, "< >   Menu");
    lv_obj_align(bottom, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static void screen_update_dashboard(void)
{
    float temp = 25.0f;
    ds18b20_read(&temp);
    char buf[64];
    snprintf(buf, sizeof(buf), "Temperatura: %.1f C", (double)temp);
    lv_label_set_text(temp_label, buf);

    int on_count = 0;
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        if (g_gs.system_state >= SYSTEM_STATE_SAFE_OFF) on_count++;
    }
    snprintf(buf, sizeof(buf), "Tomadas: %d/%d", on_count, PLUG_COUNT_TOTAL);
    lv_label_set_text(plugs_label, buf);

    snprintf(buf, sizeof(buf), "Alarmes: %d", g_gs.active_alerts_count);
    lv_label_set_text(alerts_label, buf);

    snprintf(buf, sizeof(buf), "Energia: %.0f W", (double)g_pzem.power_w);
    lv_label_set_text(energy_label, buf);
}

static void __attribute__((constructor)) register_dashboard(void)
{
    ui_screen_register(SCREEN_DASHBOARD, screen_init_dashboard, screen_update_dashboard);
}
