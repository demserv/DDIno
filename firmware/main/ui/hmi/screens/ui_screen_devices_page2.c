// @requirement RF-PLUG-001 a RF-PLUG-010 Tela de dispositivos P06-P10
// @requirement RF-PLUG-014 Desbloqueio manual de plugue via UI
#include "ui_screen_devices_page2.h"
#include "../ui_theme.h"
#include "../components/ui_device_row.h"
#include "../ui_events.h"
#include "../ui_preset_picker.h"
#include "plug_model.h"

static ui_device_row_t rows[5];
static uint8_t s_plug_ids[5] = {6, 7, 8, 9, 10};
static lv_obj_t *s_parent = NULL;

static void unblock_cb(lv_event_t *e)
{
    uint8_t *pid = (uint8_t *)lv_event_get_user_data(e);
    if (pid) ui_events_unblock_plug(*pid);
}

static void toggle_cb(lv_event_t *e)
{
    uint8_t *pid = (uint8_t *)lv_event_get_user_data(e);
    if (pid) ui_events_toggle_plug(*pid);
}

static void preset_cb(lv_event_t *e)
{
    uint8_t *pid = (uint8_t *)lv_event_get_user_data(e);
    if (pid && s_parent) {
        ui_preset_picker_show(s_parent, (plug_id_t)(*pid));
    }
}

void ui_screen_devices_page2_create(lv_obj_t *parent, ui_root_vm_t *vm)
{
    s_parent = parent;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Dispositivos 2/2");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_pos(title, 168, 5);

    static const int y_positions[5] = {32, 78, 124, 170, 216};
    for (int i = 0; i < 5; i++) {
        ui_device_row_create(&rows[i], parent, 10, y_positions[i]);
        ui_device_row_set_unblock_cb(&rows[i], unblock_cb, &s_plug_ids[i]);
        ui_device_row_set_toggle_cb(&rows[i], toggle_cb, &s_plug_ids[i]);
        ui_device_row_set_preset_cb(&rows[i], preset_cb, &s_plug_ids[i]);
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
