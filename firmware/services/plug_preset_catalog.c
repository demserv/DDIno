/* @requirement RF-PLUG-002 Nomes e icones predefinidos para P03..P10
 * Decisao normativa do usuario — 2026-06-30 (item 7). */
#include "services/plug_preset_catalog.h"

#include <string.h>

static const plug_preset_t s_presets[] = {
    { "Bomba Recalque",   LV_SYMBOL_REFRESH, PLUG_TYPE_BOMBA,     PLUG_PRESET_BOMBA_RECALQUE },
    { "Circulacao",       LV_SYMBOL_LOOP,    PLUG_TYPE_BOMBA,     PLUG_PRESET_CIRCULACAO },
    { "Bomba Reposicao ATO", LV_SYMBOL_DOWNLOAD, PLUG_TYPE_BOMBA, PLUG_PRESET_BOMBA_ATO },
    { "Iluminacao",       LV_SYMBOL_IMAGE,   PLUG_TYPE_LUZ,       PLUG_PRESET_ILUMINACAO },
    { "Skimmer",          LV_SYMBOL_SHUFFLE, PLUG_TYPE_FILTRO,    PLUG_PRESET_SKIMMER },
    { "Aquecedor",        LV_SYMBOL_UP,      PLUG_TYPE_AQUECEDOR, PLUG_PRESET_AQUECEDOR },
    { "Cooler",           LV_SYMBOL_DOWN,    PLUG_TYPE_COOLER,    PLUG_PRESET_COOLER },
};

uint8_t plug_preset_count(void)
{
    return (uint8_t)(sizeof(s_presets) / sizeof(s_presets[0]));
}

const plug_preset_t *plug_preset_get(uint8_t index)
{
    if (index >= plug_preset_count()) return NULL;
    return &s_presets[index];
}

const plug_preset_t *plug_preset_find_by_id(plug_preset_id_t id)
{
    for (uint8_t i = 0; i < plug_preset_count(); i++) {
        if (s_presets[i].id == id) return &s_presets[i];
    }
    return NULL;
}

const char *plug_preset_icon_for_name(const char *name)
{
    if (!name || !name[0]) return LV_SYMBOL_SETTINGS;

    for (uint8_t i = 0; i < plug_preset_count(); i++) {
        if (strcmp(name, s_presets[i].name) == 0) {
            return s_presets[i].icon;
        }
    }
    return LV_SYMBOL_SETTINGS;
}
