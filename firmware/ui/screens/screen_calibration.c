// @requirement RNF-CALIB-001 Calibração assistida de sensores
#include "../ui_screens.h"
#include "global_state.h"
#include "param_catalog.h"
#include "services/config_manager.h"
#include "lvgl.h"

static lv_obj_t *calib_labels[14];

extern global_state_t g_gs;

static void screen_init_calibration(lv_obj_t *parent)
{
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Calibracao");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);

    const int col1_x = 10;
    const int col2_x = 240;
    const int row_h = 23;

    for (int i = 0; i < 14; i++) {
        int col = (i < 7) ? col1_x : col2_x;
        int row = (i % 7);
        calib_labels[i] = lv_label_create(parent);
        lv_label_set_text(calib_labels[i], "--");
        lv_obj_align(calib_labels[i], LV_ALIGN_TOP_LEFT, col, 35 + row * row_h);
    }

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "< > Navegar | API: calibrar");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void screen_update_calibration(void)
{
    const calibration_params_storage_t *cp = config_get_calibration();
    char buf[64];

    snprintf(buf, sizeof(buf), "ATO Zero ADC: %ld", (long)cp->ato_zero_offset_adc);
    lv_label_set_text(calib_labels[0], buf);

    snprintf(buf, sizeof(buf), "Temp Offset: %+.1f C", (double)cp->temp_offset_c);
    lv_label_set_text(calib_labels[1], buf);

    snprintf(buf, sizeof(buf), "P01 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[0]);
    lv_label_set_text(calib_labels[2], buf);

    snprintf(buf, sizeof(buf), "P02 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[1]);
    lv_label_set_text(calib_labels[3], buf);

    snprintf(buf, sizeof(buf), "P03 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[2]);
    lv_label_set_text(calib_labels[4], buf);

    snprintf(buf, sizeof(buf), "P04 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[3]);
    lv_label_set_text(calib_labels[5], buf);

    snprintf(buf, sizeof(buf), "P05 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[4]);
    lv_label_set_text(calib_labels[6], buf);

    snprintf(buf, sizeof(buf), "P06 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[5]);
    lv_label_set_text(calib_labels[7], buf);

    snprintf(buf, sizeof(buf), "P07 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[6]);
    lv_label_set_text(calib_labels[8], buf);

    snprintf(buf, sizeof(buf), "P08 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[7]);
    lv_label_set_text(calib_labels[9], buf);

    snprintf(buf, sizeof(buf), "P09 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[8]);
    lv_label_set_text(calib_labels[10], buf);

    snprintf(buf, sizeof(buf), "P10 Zero: %.0f mV", (double)cp->acs712_zero_offset_mv[9]);
    lv_label_set_text(calib_labels[11], buf);
}

static void __attribute__((constructor)) register_calibration(void)
{
    ui_screen_register(SCREEN_CALIBRATION, screen_init_calibration, screen_update_calibration);
}
