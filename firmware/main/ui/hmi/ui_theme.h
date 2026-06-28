// @requirement RNF-USAB-001 Tema: 14 cores, 6 raios, 6 pads, 6 fontes Montserrat
#pragma once

#include "lvgl.h"

#define UI_SCREEN_WIDTH      480
#define UI_SCREEN_HEIGHT     320

#define UI_TOPBAR_HEIGHT     31
#define UI_FOOTER_HEIGHT     31
#define UI_CONTENT_Y         UI_TOPBAR_HEIGHT
#define UI_CONTENT_HEIGHT    (UI_SCREEN_HEIGHT - UI_TOPBAR_HEIGHT - UI_FOOTER_HEIGHT)

#define UI_COLOR_BG          lv_color_hex(0x000000)
#define UI_COLOR_TOPBAR      lv_color_hex(0x11121B)
#define UI_COLOR_FOOTER      lv_color_hex(0x11121B)
#define UI_COLOR_PANEL       lv_color_hex(0x171C2A)
#define UI_COLOR_PANEL_2     lv_color_hex(0x1D2333)
#define UI_COLOR_PANEL_3     lv_color_hex(0x22283A)

#define UI_COLOR_TEXT_MAIN   lv_color_hex(0xF4F6FF)
#define UI_COLOR_TEXT_MUTED  lv_color_hex(0x8E93A8)
#define UI_COLOR_TEXT_DIM    lv_color_hex(0x646A80)

#define UI_COLOR_OK          lv_color_hex(0x00E676)
#define UI_COLOR_WARN        lv_color_hex(0xFFC928)
#define UI_COLOR_HIGH        lv_color_hex(0xFF9F1C)
#define UI_COLOR_CRITICAL    lv_color_hex(0xFF3B30)
#define UI_COLOR_INFO        lv_color_hex(0x3A86FF)
#define UI_COLOR_CYAN        lv_color_hex(0x00DFF0)
#define UI_COLOR_BLUE        lv_color_hex(0x2F8CFF)
#define UI_COLOR_PURPLE      lv_color_hex(0xC77DFF)
#define UI_COLOR_YELLOW      lv_color_hex(0xFFE600)
#define UI_COLOR_DISABLED    lv_color_hex(0x555555)

#define UI_RADIUS_SMALL      4
#define UI_RADIUS_CARD       6
#define UI_RADIUS_BUTTON     6
#define UI_RADIUS_TILE       8

#define UI_PAD_2             2
#define UI_PAD_4             4
#define UI_PAD_6             6
#define UI_PAD_8             8
#define UI_PAD_10            10
#define UI_PAD_12            12

#define UI_FONT_SMALL        (&lv_font_montserrat_10)
#define UI_FONT_NORMAL       (&lv_font_montserrat_12)
#define UI_FONT_MEDIUM       (&lv_font_montserrat_14)
#define UI_FONT_TITLE        (&lv_font_montserrat_18)
#define UI_FONT_BIG          (&lv_font_montserrat_28)
#define UI_FONT_HUGE         (&lv_font_montserrat_48)

void ui_theme_init(void);

lv_style_t *ui_theme_get_topbar(void);
lv_style_t *ui_theme_get_footer(void);
lv_style_t *ui_theme_get_card(void);
lv_style_t *ui_theme_get_button(void);
lv_style_t *ui_theme_get_label(void);
lv_style_t *ui_theme_get_badge(void);
lv_style_t *ui_theme_get_alert(void);
lv_style_t *ui_theme_get_overlay(void);
