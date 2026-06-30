// @requirement RF-UI-STATUS-001 Topbar com WiFi, datetime, botão FEED
#include "ui_topbar.h"
#include "../ui_theme.h"
#include "../ui_events.h"

#include <stdio.h>
#include <string.h>

/* @requirement RF-UI-SHORTCUT-001 / RF-FEED-001 Ativação de Feed exige confirmação
 * explícita: o atalho/botão abre um diálogo; só o "Confirmar" dispara o evento. */
static void feed_confirm_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    const char *txt = lv_msgbox_get_active_btn_text(mbox);
    if (txt && strcmp(txt, "Confirmar") == 0) {
        ui_events_emit(UI_EVENT_REQUEST_FEED_MODE);
    }
    lv_msgbox_close(mbox);
}

/* @requirement RF-UI-SHORTCUT-001 / RF-FEED-001 Diálogo de confirmação reutilizável
 * (touch topbar e tecla FEED do keypad). */
void ui_topbar_show_feed_confirm(void)
{
    static const char *btns[] = {"Confirmar", "Cancelar", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, "Feed Mode",
        "Desligar a bomba e iniciar o Feed Mode?", btns, false);
    lv_obj_add_event_cb(mbox, feed_confirm_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(mbox);
}

static void feed_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_topbar_show_feed_confirm();
}

void ui_topbar_create(ui_topbar_t *bar, lv_obj_t *parent)
{
    bar->root = lv_obj_create(parent);
    lv_obj_set_size(bar->root, 480, 31);
    lv_obj_set_pos(bar->root, 0, 0);
    lv_obj_set_style_bg_color(bar->root, UI_COLOR_TOPBAR, 0);
    lv_obj_set_style_border_width(bar->root, 0, 0);
    lv_obj_set_style_radius(bar->root, 0, 0);
    lv_obj_clear_flag(bar->root, LV_OBJ_FLAG_SCROLLABLE);

    bar->wifi_label = lv_label_create(bar->root);
    lv_label_set_text(bar->wifi_label, "WiFi");
    lv_obj_set_style_text_font(bar->wifi_label, UI_FONT_NORMAL, 0);
    lv_obj_set_pos(bar->wifi_label, 8, 7);

    bar->datetime_label = lv_label_create(bar->root);
    lv_label_set_text(bar->datetime_label, "--/--/---- --:--");
    lv_obj_set_style_text_font(bar->datetime_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(bar->datetime_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_size(bar->datetime_label, 220, 14);
    lv_obj_set_pos(bar->datetime_label, 130, 7);
    lv_obj_set_style_text_align(bar->datetime_label, LV_TEXT_ALIGN_CENTER, 0);

    bar->feed_btn = lv_btn_create(bar->root);
    lv_obj_set_size(bar->feed_btn, 90, 24);
    lv_obj_set_pos(bar->feed_btn, 382, 4);
    lv_obj_set_style_radius(bar->feed_btn, UI_RADIUS_BUTTON, 0);
    lv_obj_set_style_bg_color(bar->feed_btn, UI_COLOR_WARN, 0);
    lv_obj_add_event_cb(bar->feed_btn, feed_btn_cb, LV_EVENT_CLICKED, NULL);

    bar->feed_label = lv_label_create(bar->feed_btn);
    lv_label_set_text(bar->feed_label, "FEED");
    lv_obj_center(bar->feed_label);
    lv_obj_set_style_text_font(bar->feed_label, UI_FONT_NORMAL, 0);

    /* @requirement RF-UI-MUTE-001 Indicador vermelho ao lado do WiFi (mesmo tamanho). */
    bar->mute_label = lv_label_create(bar->root);
    lv_label_set_text(bar->mute_label, LV_SYMBOL_MUTE);
    lv_obj_set_style_text_font(bar->mute_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(bar->mute_label, UI_COLOR_CRITICAL, 0);
    lv_obj_set_pos(bar->mute_label, 52, 7);
    lv_obj_add_flag(bar->mute_label, LV_OBJ_FLAG_HIDDEN);

    /* @requirement RF-UI-STATUS-001 Indicadores compactos: SD ausente, manutenção,
     * self-test falho e pausa do carrossel. */
    bar->status_label = lv_label_create(bar->root);
    lv_label_set_text(bar->status_label, "");
    lv_obj_set_style_text_font(bar->status_label, UI_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(bar->status_label, UI_COLOR_WARN, 0);
    lv_obj_set_pos(bar->status_label, 350, 7);
}

void ui_topbar_update(ui_topbar_t *bar, const ui_topbar_vm_t *vm)
{
    if (vm->wifi_ok) {
        lv_label_set_text(bar->wifi_label, "WiFi");
        lv_obj_set_style_text_color(bar->wifi_label, UI_COLOR_OK, 0);
    } else {
        lv_label_set_text(bar->wifi_label, "WiFi");
        lv_obj_set_style_text_color(bar->wifi_label, UI_COLOR_TEXT_DIM, 0);
    }

    lv_label_set_text(bar->datetime_label, vm->datetime_text);

    if (vm->mute_active) {
        lv_obj_clear_flag(bar->mute_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(bar->mute_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (vm->feed_mode_active) {
        lv_obj_set_style_bg_color(bar->feed_btn, lv_color_hex(0xFF6B00), 0);
        lv_obj_add_state(bar->feed_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_set_style_bg_color(bar->feed_btn, UI_COLOR_WARN, 0);
        lv_obj_clear_state(bar->feed_btn, LV_STATE_DISABLED);
    }

    {
        char status[48] = "";
        int pos = 0;
        if (!vm->sd_ok) {
            pos += snprintf(status + pos, sizeof(status) - (size_t)pos, "SD ");
        }
        if (vm->maintenance_active) {
            pos += snprintf(status + pos, sizeof(status) - (size_t)pos, "MNT ");
        }
        if (vm->selftest_failed) {
            pos += snprintf(status + pos, sizeof(status) - (size_t)pos, "TST ");
        }
        if (vm->carousel_paused) {
            pos += snprintf(status + pos, sizeof(status) - (size_t)pos, "|| ");
        }
        if (status[0] != '\0') {
            lv_label_set_text(bar->status_label, status);
            lv_obj_clear_flag(bar->status_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(bar->status_label, "");
            lv_obj_add_flag(bar->status_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
