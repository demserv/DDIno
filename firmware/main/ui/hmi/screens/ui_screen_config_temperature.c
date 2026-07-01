// @requirement RF-THERMAL-003 a RF-THERMAL-007 Tela de configuração de parâmetros térmicos
#include "ui_screen_config_temperature.h"
#include "../ui_theme.h"
#include "../ui_events.h"
#include "../ui_screen_manager.h"
#include "config_manager.h"
#include <stdio.h>

typedef struct {
    lv_obj_t *value_lbl;
    float *field;
    float step;
    float min_v;
    float max_v;
} temp_row_t;

static temp_row_t rows[6];
static thermal_params_storage_t s_edit;

static void refresh_row(temp_row_t *row)
{
    if (!row || !row->value_lbl || !row->field) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f C", (double)*row->field);
    lv_label_set_text(row->value_lbl, buf);
}

static void minus_cb(lv_event_t *e)
{
    temp_row_t *row = (temp_row_t *)lv_event_get_user_data(e);
    if (!row) return;
    *row->field -= row->step;
    if (*row->field < row->min_v) *row->field = row->min_v;
    refresh_row(row);
}

static void plus_cb(lv_event_t *e)
{
    temp_row_t *row = (temp_row_t *)lv_event_get_user_data(e);
    if (!row) return;
    *row->field += row->step;
    if (*row->field > row->max_v) *row->field = row->max_v;
    refresh_row(row);
}

static void save_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_events_emit(UI_EVENT_REQUEST_SAVE_THERMAL_CONFIG);
}

static void cancel_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_manager_show(UI_SCREEN_CONFIG_HUB);
}

static void load_from_config(void)
{
    const thermal_params_storage_t *tp = config_get_thermal();
    if (tp) {
        s_edit = *tp;
    }
}

void ui_screen_config_temperature_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    load_from_config();

    lv_obj_t *breadcrumb = lv_label_create(parent);
    lv_label_set_text(breadcrumb, "< Config > Temperatura");
    lv_obj_set_style_text_font(breadcrumb, UI_FONT_SMALL, 0);
    lv_obj_set_pos(breadcrumb, 14, 6);

    static const char *labels[] = {
        "Setpoint", "Min", "Max", "Critica", "Histerese", "Extrema"
    };
    static float *fields[] = {
        &s_edit.temp_normal_c, &s_edit.temp_min_c, &s_edit.temp_max_c,
        &s_edit.temp_critical_c, &s_edit.hysteresis_c, &s_edit.temp_extreme_c
    };

    for (int i = 0; i < 6; i++) {
        int y = 36 + i * 34;
        lv_obj_t *panel = lv_obj_create(parent);
        lv_obj_set_size(panel, 460, 30);
        lv_obj_set_pos(panel, 10, y);
        lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
        lv_obj_set_style_radius(panel, UI_RADIUS_CARD, 0);

        lv_obj_t *lbl = lv_label_create(panel);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_pos(lbl, 8, 6);

        rows[i].field = fields[i];
        rows[i].step = (i == 4) ? 0.1f : 0.5f;
        rows[i].min_v = (i == 5) ? 20.0f : 10.0f;
        rows[i].max_v = 45.0f;
        rows[i].value_lbl = lv_label_create(panel);
        lv_obj_set_style_text_color(rows[i].value_lbl, UI_COLOR_CYAN, 0);
        lv_obj_set_pos(rows[i].value_lbl, 200, 6);

        lv_obj_t *mb = lv_btn_create(panel);
        lv_obj_set_size(mb, 28, 22);
        lv_obj_set_pos(mb, 320, 4);
        lv_obj_add_event_cb(mb, minus_cb, LV_EVENT_CLICKED, &rows[i]);
        lv_obj_t *ml = lv_label_create(mb);
        lv_label_set_text(ml, "-");
        lv_obj_center(ml);

        lv_obj_t *pb = lv_btn_create(panel);
        lv_obj_set_size(pb, 28, 22);
        lv_obj_set_pos(pb, 360, 4);
        lv_obj_add_event_cb(pb, plus_cb, LV_EVENT_CLICKED, &rows[i]);
        lv_obj_t *pl = lv_label_create(pb);
        lv_label_set_text(pl, "+");
        lv_obj_center(pl);

        refresh_row(&rows[i]);
    }

    lv_obj_t *save_btn = lv_btn_create(parent);
    lv_obj_set_size(save_btn, 120, 26);
    lv_obj_set_pos(save_btn, 110, 250);
    lv_obj_add_event_cb(save_btn, save_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Salvar");
    lv_obj_center(save_label);

    lv_obj_t *cancel_btn = lv_btn_create(parent);
    lv_obj_set_size(cancel_btn, 120, 26);
    lv_obj_set_pos(cancel_btn, 250, 250);
    lv_obj_add_event_cb(cancel_btn, cancel_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Voltar");
    lv_obj_center(cancel_label);
}

bool ui_screen_config_temperature_save(void)
{
    if (s_edit.temp_min_c >= s_edit.temp_max_c) return false;
    if (s_edit.temp_normal_c < s_edit.temp_min_c || s_edit.temp_normal_c > s_edit.temp_max_c) {
        return false;
    }
    return config_set_thermal(&s_edit) == ESP_OK;
}

void ui_screen_config_temperature_update(ui_root_vm_t *vm)
{
    (void)vm;
    load_from_config();
    for (int i = 0; i < 6; i++) {
        refresh_row(&rows[i]);
    }
}
