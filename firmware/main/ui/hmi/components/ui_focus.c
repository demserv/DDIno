#include "ui_focus.h"
#include "../ui_theme.h"

void ui_focus_apply(lv_obj_t *obj)
{
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, UI_COLOR_CYAN, 0);
}

void ui_focus_clear(lv_obj_t *obj)
{
    lv_obj_set_style_border_width(obj, 0, 0);
}
