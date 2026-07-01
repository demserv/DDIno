#ifndef FIRMWARE_SERVICES_PLUG_PRESET_CATALOG_H
#define FIRMWARE_SERVICES_PLUG_PRESET_CATALOG_H

#include <stdint.h>
#include "plug_model.h"
#include "lvgl.h"

typedef enum {
    PLUG_PRESET_BOMBA_RECALQUE = 0,
    PLUG_PRESET_CIRCULACAO,
    PLUG_PRESET_BOMBA_ATO,
    PLUG_PRESET_ILUMINACAO,
    PLUG_PRESET_SKIMMER,
    PLUG_PRESET_AQUECEDOR,
    PLUG_PRESET_COOLER
} plug_preset_id_t;

typedef struct {
    const char   *name;
    const char   *icon;
    plug_type_t   type;
    plug_preset_id_t id;
} plug_preset_t;

uint8_t plug_preset_count(void);
const plug_preset_t *plug_preset_get(uint8_t index);
const plug_preset_t *plug_preset_find_by_id(plug_preset_id_t id);
const char *plug_preset_icon_for_name(const char *name);

#endif
