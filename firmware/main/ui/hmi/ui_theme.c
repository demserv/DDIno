// @requirement RNF-USAB-001 Tema centralizado com cores, raios, pads e fontes
#include "ui_theme.h"

static lv_style_t s_style_topbar;
static lv_style_t s_style_footer;
static lv_style_t s_style_card;
static lv_style_t s_style_button;
static lv_style_t s_style_label;
static lv_style_t s_style_badge;
static lv_style_t s_style_alert;
static lv_style_t s_style_overlay;
static bool s_theme_ready;

lv_style_t *ui_theme_get_topbar(void) { return &s_style_topbar; }
lv_style_t *ui_theme_get_footer(void) { return &s_style_footer; }
lv_style_t *ui_theme_get_card(void) { return &s_style_card; }
lv_style_t *ui_theme_get_button(void) { return &s_style_button; }
lv_style_t *ui_theme_get_label(void) { return &s_style_label; }
lv_style_t *ui_theme_get_badge(void) { return &s_style_badge; }
lv_style_t *ui_theme_get_alert(void) { return &s_style_alert; }
lv_style_t *ui_theme_get_overlay(void) { return &s_style_overlay; }

void ui_theme_init(void)
{
    if (s_theme_ready) {
        return;
    }

    lv_style_init(&s_style_topbar);
    lv_style_set_bg_color(&s_style_topbar, UI_COLOR_TOPBAR);
    lv_style_set_bg_opa(&s_style_topbar, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_topbar, 0);
    lv_style_set_pad_all(&s_style_topbar, UI_PAD_4);
    lv_style_set_text_color(&s_style_topbar, UI_COLOR_TEXT_MAIN);
    lv_style_set_text_font(&s_style_topbar, UI_FONT_NORMAL);

    lv_style_init(&s_style_footer);
    lv_style_set_bg_color(&s_style_footer, UI_COLOR_FOOTER);
    lv_style_set_bg_opa(&s_style_footer, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_footer, 0);
    lv_style_set_pad_all(&s_style_footer, UI_PAD_4);
    lv_style_set_text_color(&s_style_footer, UI_COLOR_TEXT_MAIN);
    lv_style_set_text_font(&s_style_footer, UI_FONT_NORMAL);

    lv_style_init(&s_style_card);
    lv_style_set_bg_color(&s_style_card, UI_COLOR_PANEL);
    lv_style_set_bg_opa(&s_style_card, LV_OPA_COVER);
    lv_style_set_radius(&s_style_card, UI_RADIUS_CARD);
    lv_style_set_border_width(&s_style_card, 0);
    lv_style_set_pad_all(&s_style_card, UI_PAD_8);

    lv_style_init(&s_style_button);
    lv_style_set_bg_color(&s_style_button, UI_COLOR_PANEL_2);
    lv_style_set_bg_opa(&s_style_button, LV_OPA_COVER);
    lv_style_set_radius(&s_style_button, UI_RADIUS_BUTTON);
    lv_style_set_border_width(&s_style_button, 0);
    lv_style_set_pad_all(&s_style_button, UI_PAD_8);
    lv_style_set_text_color(&s_style_button, UI_COLOR_TEXT_MAIN);
    lv_style_set_text_font(&s_style_button, UI_FONT_MEDIUM);

    lv_style_init(&s_style_label);
    lv_style_set_text_color(&s_style_label, UI_COLOR_TEXT_MAIN);
    lv_style_set_text_font(&s_style_label, UI_FONT_NORMAL);

    lv_style_init(&s_style_badge);
    lv_style_set_bg_color(&s_style_badge, UI_COLOR_PANEL_3);
    lv_style_set_bg_opa(&s_style_badge, LV_OPA_COVER);
    lv_style_set_radius(&s_style_badge, UI_RADIUS_SMALL);
    lv_style_set_pad_hor(&s_style_badge, UI_PAD_6);
    lv_style_set_pad_ver(&s_style_badge, UI_PAD_2);
    lv_style_set_text_color(&s_style_badge, UI_COLOR_TEXT_MAIN);
    lv_style_set_text_font(&s_style_badge, UI_FONT_SMALL);

    lv_style_init(&s_style_alert);
    lv_style_set_bg_color(&s_style_alert, UI_COLOR_CRITICAL);
    lv_style_set_bg_opa(&s_style_alert, LV_OPA_30);
    lv_style_set_radius(&s_style_alert, UI_RADIUS_CARD);
    lv_style_set_border_color(&s_style_alert, UI_COLOR_CRITICAL);
    lv_style_set_border_width(&s_style_alert, 1);
    lv_style_set_pad_all(&s_style_alert, UI_PAD_6);
    lv_style_set_text_color(&s_style_alert, UI_COLOR_TEXT_MAIN);

    lv_style_init(&s_style_overlay);
    lv_style_set_bg_color(&s_style_overlay, lv_color_black());
    lv_style_set_bg_opa(&s_style_overlay, LV_OPA_70);
    lv_style_set_border_width(&s_style_overlay, 2);
    lv_style_set_border_color(&s_style_overlay, UI_COLOR_CRITICAL);
    lv_style_set_radius(&s_style_overlay, UI_RADIUS_TILE);
    lv_style_set_pad_all(&s_style_overlay, UI_PAD_12);
    lv_style_set_text_color(&s_style_overlay, UI_COLOR_TEXT_MAIN);
    lv_style_set_text_font(&s_style_overlay, UI_FONT_TITLE);

    s_theme_ready = true;
}
