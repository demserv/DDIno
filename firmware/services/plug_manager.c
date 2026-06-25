#include "services/plug_manager.h"
#include "driver_relay.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "plug_manager";

static plug_model_t s_plugs[PLUG_COUNT_TOTAL];
static bool s_thermal_heater = false;
static bool s_thermal_cooler = false;
static uint64_t s_last_toggle_ms[PLUG_COUNT_TOTAL] = {0};

static const char *s_default_names[PLUG_COUNT_TOTAL] = {
    "Bomba ATO", "Aquecedor", "Cooler", "Filtro",
    "Aireador", "Luz", "P07", "P08", "P09", "P10"
};

static plug_type_t s_default_types[PLUG_COUNT_TOTAL] = {
    PLUG_TYPE_BOMBA, PLUG_TYPE_AQUECEDOR, PLUG_TYPE_COOLER, PLUG_TYPE_FILTRO,
    PLUG_TYPE_AIREADOR, PLUG_TYPE_LUZ, PLUG_TYPE_OUTRO, PLUG_TYPE_OUTRO,
    PLUG_TYPE_OUTRO, PLUG_TYPE_OUTRO
};

static bool s_in_safe_off = false;

void plug_manager_init(void)
{
    memset(s_plugs, 0, sizeof(s_plugs));
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        plug_model_t *p = &s_plugs[i];
        p->id = (plug_id_t)(i + 1);
        strncpy(p->name, s_default_names[i], sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
        p->type = s_default_types[i];
        p->mode = PLUG_MODE_AUTO;
        p->command_allowed = true;
        p->blocked_by_safe_state = false;
        p->monitor_only_blocked = false;
        p->effective_state = PLUG_EFFECTIVE_STATE_OFF;
        p->visual_state = PLUG_VISUAL_STATE_OFF_NORMAL;
        p->is_critical = (p->type == PLUG_TYPE_AQUECEDOR || p->type == PLUG_TYPE_COOLER);
        p->current_limit_a = 10.0f;
        p->min_on_time_s = 60;
        p->min_off_time_s = 30;
        p->timer_on_s = 0;
        p->timer_off_s = 0;
        p->delay_seconds = 0;
        p->overcurrent_count = 0;
        p->energy_wh_today = 0;
        p->energy_wh_24h = 0;
        p->energy_wh_total = 0;
        p->fator_curto = 3.0f;
        p->tempo_deteccao_curto_ms = 500;
    }
    s_in_safe_off = false;
    ESP_LOGI(TAG, "Plug manager initialized, %d plugs", PLUG_COUNT_TOTAL);
}

void plug_manager_tick(uint64_t now_s, system_state_t sys_state, bool feed_active)
{
    s_in_safe_off = (sys_state >= SYSTEM_STATE_SAFE_OFF);

    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        plug_model_t *p = &s_plugs[i];
        bool target_on = false;

        if (s_in_safe_off) {
            target_on = false;
            p->blocked_by_safe_state = true;
        } else if (feed_active && i < 4) {
            target_on = false;
        } else {
            p->blocked_by_safe_state = false;

            switch (p->mode) {
                case PLUG_MODE_AUTO: {
                    if (p->type == PLUG_TYPE_AQUECEDOR) {
                        target_on = s_thermal_heater;
                    } else if (p->type == PLUG_TYPE_COOLER) {
                        target_on = s_thermal_cooler;
                    } else if (p->type == PLUG_TYPE_FILTRO || p->type == PLUG_TYPE_BOMBA
                               || p->type == PLUG_TYPE_AIREADOR) {
                        target_on = true;
                    } else {
                        target_on = relay_get(p->id);
                    }
                    break;
                }
                case PLUG_MODE_MANUAL:
                    target_on = relay_get(p->id);
                    break;

                case PLUG_MODE_TIMER: {
                    uint32_t day_seconds = (uint32_t)(now_s % 86400ULL);
                    if (p->timer_on_s < p->timer_off_s) {
                        target_on = (day_seconds >= p->timer_on_s && day_seconds < p->timer_off_s);
                    } else {
                        target_on = (day_seconds >= p->timer_on_s || day_seconds < p->timer_off_s);
                    }
                    break;
                }
                case PLUG_MODE_DELAY: {
                    bool requested = relay_get(p->id);
                    uint64_t now_ms = now_s * 1000ULL;
                    uint64_t elapsed = now_ms - s_last_toggle_ms[i];
                    if (elapsed < (uint64_t)p->delay_seconds * 1000ULL) {
                        target_on = !requested;
                    } else {
                        target_on = requested;
                    }
                    break;
                }
                default:
                    target_on = relay_get(p->id);
                    break;
            }
        }

        bool current = relay_get(p->id);
        if (target_on != current) {
            relay_set(p->id, target_on);
            s_last_toggle_ms[i] = now_s * 1000ULL;
        }

        p->effective_state = target_on ? PLUG_EFFECTIVE_STATE_ON : PLUG_EFFECTIVE_STATE_OFF;
        p->command_allowed = !s_in_safe_off;
    }
}

void plug_manager_set_thermal_request(plug_id_t id, bool heater_on, bool cooler_on)
{
    s_thermal_heater = heater_on;
    s_thermal_cooler = cooler_on;
}

plug_mode_t plug_manager_get_mode(plug_id_t id)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return PLUG_MODE_MANUAL;
    return s_plugs[id - 1].mode;
}

esp_err_t plug_manager_set_mode(plug_id_t id, plug_mode_t mode)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return ESP_ERR_INVALID_ARG;
    if (mode > PLUG_MODE_OVERRIDE) return ESP_ERR_INVALID_ARG;
    s_plugs[id - 1].mode = mode;
    return ESP_OK;
}

bool plug_manager_get_effective_state(plug_id_t id)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return false;
    return relay_get(id);
}

esp_err_t plug_manager_toggle(plug_id_t id, bool on)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return ESP_ERR_INVALID_ARG;
    if (s_in_safe_off) return ESP_ERR_INVALID_STATE;
    if (s_plugs[id - 1].monitor_only_blocked) return ESP_ERR_INVALID_STATE;

    relay_set(id, on);
    s_plugs[id - 1].effective_state = on ? PLUG_EFFECTIVE_STATE_ON : PLUG_EFFECTIVE_STATE_OFF;
    return ESP_OK;
}

uint8_t plug_manager_active_count(void)
{
    uint8_t count = 0;
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        if (relay_get(s_plugs[i].id)) count++;
    }
    return count;
}

plug_model_t *plug_manager_get(plug_id_t id)
{
    if (id < 1 || id > PLUG_COUNT_TOTAL) return NULL;
    return &s_plugs[id - 1];
}

void plug_manager_apply_safe_off(void)
{
    s_in_safe_off = true;
    relay_all_off();
    for (int i = 0; i < PLUG_COUNT_TOTAL; i++) {
        s_plugs[i].effective_state = PLUG_EFFECTIVE_STATE_OFF;
        s_plugs[i].visual_state = PLUG_VISUAL_STATE_OFF_FAULT;
        s_plugs[i].blocked_by_safe_state = true;
    }
}
