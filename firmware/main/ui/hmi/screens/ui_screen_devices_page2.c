#include "ui_screen_devices_page2.h"
#include "../ui_theme.h"
#include "../components/ui_device_row.h"

static ui_device_row_t rows[5];

void ui_screen_devices_page2_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Dispositivos 2/2");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 168, 5);

    static const int y_positions[5] = {32, 78, 124, 170, 216};
    for (int i = 0; i < 5; i++) {
        ui_device_row_create(&rows[i], parent, 10, y_positions[i]);
        ui_device_row_update(&rows[i], &vm->devices.plugs[5 + i]);
    }
}

void ui_screen_devices_page2_update(ui_root_vm_t *vm)
{
    if (!vm) return;
    for (int i = 0; i < 5 && (5 + i) < (int)vm->devices.plug_count; i++) {
        ui_device_row_update(&rows[i], &vm->devices.plugs[5 + i]);
    }
}
