#ifndef FIRMWARE_MAIN_UI_HMI_UI_PROFILE_NAME_EDITOR_H
#define FIRMWARE_MAIN_UI_HMI_UI_PROFILE_NAME_EDITOR_H

#include "lvgl.h"

typedef void (*ui_profile_name_editor_cb_t)(const char *name, void *user_data);

void ui_profile_name_editor_show(lv_obj_t *parent, const char *initial,
                                 ui_profile_name_editor_cb_t cb, void *user_data);
void ui_profile_name_editor_close(void);

#endif
