// @requirement RF-LOG-001 a RF-LOG-020 Tela de logs do sistema
#include "ui_screen_logs.h"
#include "../ui_theme.h"
#include "log_manager.h"
#include <stdio.h>

#define LOG_DISPLAY_LINES 12

static lv_obj_t *log_labels[LOG_DISPLAY_LINES];

void ui_screen_logs_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    (void)vm;
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Logs");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 210, 6);

    for (int i = 0; i < LOG_DISPLAY_LINES; i++) {
        log_labels[i] = lv_label_create(parent);
        lv_obj_set_pos(log_labels[i], 10, 35 + i * 22);
        lv_obj_set_size(log_labels[i], 460, 22);
        lv_label_set_long_mode(log_labels[i], LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_font(log_labels[i], UI_FONT_NORMAL, 0);
        lv_obj_set_style_text_color(log_labels[i], UI_COLOR_TEXT_MAIN, 0);
        lv_label_set_text(log_labels[i], "");
    }

    ui_screen_logs_update(vm);
}

void ui_screen_logs_update(ui_root_vm_t *vm)
{
    (void)vm;

    log_record_t records[LOG_DISPLAY_LINES];
    uint32_t actual = 0;

    esp_err_t err = log_manager_get_recent(records, LOG_DISPLAY_LINES, &actual);
    if (err != ESP_OK) {
        lv_label_set_text(log_labels[0], "Log manager indisponivel");
        for (int i = 1; i < LOG_DISPLAY_LINES; i++) {
            lv_label_set_text(log_labels[i], "");
        }
        return;
    }

    if (actual == 0) {
        lv_label_set_text(log_labels[0], "Nenhum log registrado");
        for (int i = 1; i < LOG_DISPLAY_LINES; i++) {
            lv_label_set_text(log_labels[i], "");
        }
        return;
    }

    for (uint32_t i = 0; i < LOG_DISPLAY_LINES; i++) {
        if (i < actual) {
            const log_record_t *r = &records[actual - 1 - i];
            char buf[128];
            snprintf(buf, sizeof(buf), "[%s][%s] %s",
                     r->severity == LOG_SEVERITY_DEBUG ? "DBG" :
                     r->severity == LOG_SEVERITY_INFO ? "INF" :
                     r->severity == LOG_SEVERITY_WARNING ? "WRN" :
                     r->severity == LOG_SEVERITY_ERROR ? "ERR" : "CRI",
                     r->module, r->message);
            lv_label_set_text(log_labels[i], buf);
        } else {
            lv_label_set_text(log_labels[i], "");
        }
    }
}
