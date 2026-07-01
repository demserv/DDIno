// Editor modal de nome de perfil (textarea + teclado LVGL)
#include "ui_profile_name_editor.h"
#include "ui_theme.h"
#include "profile_manager.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_overlay = NULL;
static lv_obj_t *s_ta = NULL;
static lv_obj_t *s_kb = NULL;
static lv_obj_t *s_err_lbl = NULL;
static ui_profile_name_editor_cb_t s_done_cb = NULL;
static void *s_user_data = NULL;

static void editor_close(void)
{
    if (s_overlay) {
        lv_obj_del(s_overlay);
        s_overlay = NULL;
        s_ta = NULL;
        s_kb = NULL;
        s_err_lbl = NULL;
    }
    s_done_cb = NULL;
    s_user_data = NULL;
}

static void editor_apply(void)
{
    if (!s_ta || !s_done_cb) {
        editor_close();
        return;
    }
    const char *text = lv_textarea_get_text(s_ta);
    if (!profile_manager_is_name_valid(text)) {
        if (s_err_lbl) {
            lv_label_set_text(s_err_lbl, "Nome invalido (a-z, 0-9, _, -)");
        }
        return;
    }
    s_done_cb(text, s_user_data);
    editor_close();
}

static void cancel_cb(lv_event_t *e)
{
    (void)e;
    editor_close();
}

static void ok_cb(lv_event_t *e)
{
    (void)e;
    editor_apply();
}

static void ta_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        editor_apply();
    } else if (code == LV_EVENT_VALUE_CHANGED && s_err_lbl) {
        lv_label_set_text(s_err_lbl, "");
    }
}

void ui_profile_name_editor_close(void)
{
    editor_close();
}

void ui_profile_name_editor_show(lv_obj_t *parent, const char *initial,
                                 ui_profile_name_editor_cb_t cb, void *user_data)
{
    if (!parent || !cb) return;
    ui_profile_name_editor_close();

    s_done_cb = cb;
    s_user_data = user_data;

    s_overlay = lv_obj_create(parent);
    lv_obj_set_size(s_overlay, 480, 320);
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_70, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(s_overlay);
    lv_obj_set_size(panel, 460, 300);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, UI_COLOR_PANEL, 0);
    lv_obj_set_style_radius(panel, UI_RADIUS_CARD, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "Nome do perfil");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    s_ta = lv_textarea_create(panel);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_max_length(s_ta, PROFILE_NAME_MAX - 1);
    lv_obj_set_width(s_ta, 420);
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 34);
    lv_textarea_set_text(s_ta, initial ? initial : "");
    lv_obj_add_event_cb(s_ta, ta_cb, LV_EVENT_ALL, NULL);

    s_err_lbl = lv_label_create(panel);
    lv_obj_set_width(s_err_lbl, 420);
    lv_obj_set_style_text_color(s_err_lbl, UI_COLOR_WARN, 0);
    lv_obj_set_style_text_font(s_err_lbl, UI_FONT_SMALL, 0);
    lv_obj_align(s_err_lbl, LV_ALIGN_TOP_MID, 0, 66);

    s_kb = lv_keyboard_create(panel);
    lv_obj_set_size(s_kb, 440, 130);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, -36);
    lv_keyboard_set_mode(s_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_kb, s_ta);

    lv_obj_t *ok = lv_btn_create(panel);
    lv_obj_set_size(ok, 70, 28);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_LEFT, 12, -6);
    lv_obj_add_event_cb(ok, ok_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_l = lv_label_create(ok);
    lv_label_set_text(ok_l, "OK");
    lv_obj_center(ok_l);

    lv_obj_t *cancel = lv_btn_create(panel);
    lv_obj_set_size(cancel, 80, 28);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_RIGHT, -12, -6);
    lv_obj_add_event_cb(cancel, cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cl = lv_label_create(cancel);
    lv_label_set_text(cl, "Cancelar");
    lv_obj_center(cl);
}
