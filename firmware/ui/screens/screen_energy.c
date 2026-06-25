#include "../ui_screens.h"
#include "global_state.h"
#include "driver_acs712.h"
#include "driver_pzem.h"

#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "screen_energy";

static lv_obj_t *voltage_label = NULL;
static lv_obj_t *current_label = NULL;
static lv_obj_t *power_label = NULL;
static lv_obj_t *energy_label = NULL;
static lv_obj_t *freq_label = NULL;
static lv_obj_t *pf_label = NULL;

extern global_state_t g_gs;
extern pzem_data_t g_pzem;

static void screen_init_energy(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Energia");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 440, 260);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 10);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    voltage_label = lv_label_create(cont);
    lv_label_set_text(voltage_label, "Tensao: ---.- V");
    lv_obj_align(voltage_label, LV_ALIGN_TOP_LEFT, 10, 10);

    current_label = lv_label_create(cont);
    lv_label_set_text(current_label, "Corrente: --.-- A");
    lv_obj_align(current_label, LV_ALIGN_TOP_LEFT, 10, 45);

    power_label = lv_label_create(cont);
    lv_label_set_text(power_label, "Potencia: ---.- W");
    lv_obj_align(power_label, LV_ALIGN_TOP_LEFT, 10, 80);

    energy_label = lv_label_create(cont);
    lv_label_set_text(energy_label, "Energia: ----- Wh");
    lv_obj_align(energy_label, LV_ALIGN_TOP_LEFT, 10, 115);

    freq_label = lv_label_create(cont);
    lv_label_set_text(freq_label, "Frequencia: --.- Hz");
    lv_obj_align(freq_label, LV_ALIGN_TOP_LEFT, 10, 150);

    pf_label = lv_label_create(cont);
    lv_label_set_text(pf_label, "Fator Potencia: -.--");
    lv_obj_align(pf_label, LV_ALIGN_TOP_LEFT, 10, 185);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< >   Navegar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_energy(void)
{
    char buf[64];

    float v = g_pzem.voltage_v;
    if (v < 1.0f) v = 127.0f;
    snprintf(buf, sizeof(buf), "Tensao: %.1f V", (double)v);
    lv_label_set_text(voltage_label, buf);

    float total_i = 0;
    for (int i = 0; i < ACS712_CHANNEL_COUNT; i++) {
        float cur = 0;
        acs712_read_plug((uint8_t)i, &cur);
        total_i += cur;
    }
    total_i += g_pzem.current_a;
    snprintf(buf, sizeof(buf), "Corrente: %.2f A", (double)total_i);
    lv_label_set_text(current_label, buf);

    float p = g_pzem.power_w;
    snprintf(buf, sizeof(buf), "Potencia: %.1f W", (double)p);
    lv_label_set_text(power_label, buf);

    float e = g_pzem.energy_wh;
    snprintf(buf, sizeof(buf), "Energia: %.0f Wh", (double)e);
    lv_label_set_text(energy_label, buf);

    float f = g_pzem.frequency_hz;
    snprintf(buf, sizeof(buf), "Frequencia: %.1f Hz", (double)f);
    lv_label_set_text(freq_label, buf);

    float pf = g_pzem.pf;
    snprintf(buf, sizeof(buf), "Fator Potencia: %.2f", (double)pf);
    lv_label_set_text(pf_label, buf);
}

static void __attribute__((constructor)) register_energy(void)
{
    ui_screen_register(SCREEN_ENERGY, screen_init_energy, screen_update_energy);
}
