// @requirement RF-GLOBAL-005 Proibição de valores operacionais fixos em código
// @requirement RF-UI-WIZARD-001..005 Wizard step persistence
// @requirement RNF-CALIB-001 Calibração assistida de sensores
// @requirement RF-ATO-003 Configuração ATO com validação LOW/HIGH
#include "config_manager.h"

#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "config_root.h"
#include "driver_acs712.h"
#include "event_bus.h"

static const char *TAG = "config_mgr";

#define NVS_NS_SCHEMA    "cfg_schema"
#define NVS_NS_THERMAL   "cfg_therm"
#define NVS_NS_ATO       "cfg_ato"
#define NVS_NS_ELECTRIC  "cfg_elect"
#define NVS_NS_PLUG      "cfg_plug"
#define NVS_NS_RESTART   "cfg_rstrt"
#define NVS_NS_FEED      "cfg_feed"
#define NVS_NS_SECURITY  "cfg_sec"
#define NVS_NS_ANTIFLAP  "cfg_aflap"
#define NVS_NS_SELFTEST  "cfg_stest"
#define NVS_NS_SYSTEM    "cfg_sys"
#define NVS_NS_CALIB     "cfg_calib"
#define NVS_NS_PH        "cfg_ph"

#define NVS_KEY_BLOB     "blob"
#define NVS_KEY_VERSION  "version"

static char s_stored_version[16];

static thermal_params_storage_t  s_thermal;
static ato_params_storage_t      s_ato;
static electric_params_storage_t s_electric;
static plug_limits_storage_t     s_plug_limits;
static restart_params_storage_t  s_restart;
static feed_params_storage_t     s_feed;
static security_params_storage_t s_security;
static antiflap_params_storage_t s_antiflap;
static selftest_params_storage_t s_selftest;
static system_params_storage_t   s_system;
static calibration_params_storage_t s_calibration;
static ph_params_storage_t       s_ph;

static config_root_t s_root;

static void root_to_indiv(void)
{
    s_thermal     = s_root.thermal;
    s_ato         = s_root.ato;
    s_electric    = s_root.electric;
    s_plug_limits = s_root.plug_limits;
    s_restart     = s_root.restart;
    s_feed        = s_root.feed;
    s_security    = s_root.security;
    s_antiflap    = s_root.antiflap;
    s_selftest    = s_root.selftest;
    s_system      = s_root.system;
    s_calibration = s_root.calibration;
    s_ph          = s_root.ph;
}

static void indiv_to_root(void)
{
    s_root.thermal     = s_thermal;
    s_root.ato         = s_ato;
    s_root.electric    = s_electric;
    s_root.plug_limits = s_plug_limits;
    s_root.restart     = s_restart;
    s_root.feed        = s_feed;
    s_root.security    = s_security;
    s_root.antiflap    = s_antiflap;
    s_root.selftest    = s_selftest;
    s_root.system      = s_system;
    s_root.calibration = s_calibration;
    s_root.ph          = s_ph;
}

static esp_err_t config_publish_after(esp_err_t err)
{
    if (err == ESP_OK) {
        event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    }
    return err;
}

static bool carousel_interval_valid(uint16_t s)
{
    return s == 0 || s == 15 || s == 30 || s == 60;
}

static bool carousel_pause_valid(uint16_t s)
{
    return s == 5 || s == 10 || s == 15 || s == 30;
}

static void config_sanitize_system(void)
{
    if (!carousel_interval_valid(s_system.carousel_interval_s)) {
        s_system.carousel_interval_s = PARAM_SYSTEM_DEFAULT_CAROUSEL_INTERVAL_S;
    }
    if (!carousel_pause_valid(s_system.carousel_pause_s)) {
        s_system.carousel_pause_s = PARAM_SYSTEM_DEFAULT_CAROUSEL_PAUSE_S;
    }
}

static void set_defaults(void)
{
    s_thermal.temp_normal_c   = PARAM_THERMAL_DEFAULT_TEMP_NORMAL_C;
    s_thermal.temp_critical_c = PARAM_THERMAL_DEFAULT_TEMP_CRITICAL_C;
    s_thermal.temp_extreme_c  = PARAM_THERMAL_DEFAULT_TEMP_EXTREME_C;
    s_thermal.temp_min_c      = PARAM_THERMAL_DEFAULT_TEMP_MIN_C;
    s_thermal.temp_max_c      = PARAM_THERMAL_DEFAULT_TEMP_MAX_C;
    s_thermal.hysteresis_c    = PARAM_THERMAL_DEFAULT_HYSTERESIS_C;
    s_thermal.extreme_enabled = PARAM_THERMAL_DEFAULT_EXTREME_ENABLED;

    s_ato.enabled             = PARAM_ATO_DEFAULT_ENABLED;
    s_ato.digital_mode        = PARAM_ATO_DEFAULT_DIGITAL_MODE;
    s_ato.low_level_adc       = PARAM_ATO_DEFAULT_LOW_ADC;
    s_ato.high_level_adc      = PARAM_ATO_DEFAULT_HIGH_ADC;
    s_ato.overflow_margin_adc = PARAM_ATO_DEFAULT_OVERFLOW_ADC;
    s_ato.refill_timeout_s    = PARAM_ATO_DEFAULT_REFILL_TIMEOUT_S;

    s_electric.total_power_limit_w    = PARAM_ELECTRIC_DEFAULT_TOTAL_POWER_W;
    s_electric.per_plug_current_limit_a = PARAM_ELECTRIC_DEFAULT_PER_PLUG_CURRENT;
    s_electric.hysteresis_w           = PARAM_ELECTRIC_DEFAULT_HYSTERESIS_W;
    s_electric.overvoltage_limit_v    = PARAM_ELECTRIC_DEFAULT_OVERVOLTAGE_127V;
    s_electric.undervoltage_limit_v   = PARAM_ELECTRIC_DEFAULT_UNDERVOLTAGE_127V;
    s_electric.overvoltage_time_s     = PARAM_ELECTRIC_DEFAULT_OV_TIME_S;
    s_electric.undervoltage_time_s    = PARAM_ELECTRIC_DEFAULT_UV_TIME_S;
    s_electric.total_current_limit_a  = PARAM_ELECTRIC_DEFAULT_TOTAL_CURRENT_A;
    s_electric.total_current_time_s   = PARAM_ELECTRIC_DEFAULT_TOTAL_CURRENT_TIME_S;
    s_electric.pf_min                 = PARAM_ELECTRIC_DEFAULT_PF_MIN;
    s_electric.pf_time_s              = PARAM_ELECTRIC_DEFAULT_PF_TIME_S;
    s_electric.fator_curto            = PARAM_ELECTRIC_DEFAULT_FATOR_CURTO;
    s_electric.tempo_deteccao_curto_ms = PARAM_ELECTRIC_DEFAULT_TEMP_CURTO_MS;

    s_plug_limits.min_on_time_s       = PARAM_PLUG_DEFAULT_MIN_ON_S;
    s_plug_limits.min_off_time_s      = PARAM_PLUG_DEFAULT_MIN_OFF_S;
    for (int i = 0; i < 10; i++) {
        s_plug_limits.max_energy_wh_day[i] = (i + 1 >= 3)
            ? PARAM_PLUG_DEFAULT_MAX_ENERGY_WH_DAY : 0.0f;
    }
    for (int i = 0; i < 10; i++) {
        s_plug_limits.max_energy_wh_day[i] = (i + 1 >= 3)
            ? PARAM_PLUG_DEFAULT_MAX_ENERGY_WH_DAY : 0.0f;
    }

    s_restart.tempo_espera_religamento_s      = PARAM_RESTART_DEFAULT_ESPERA_S;
    s_restart.intervalo_religamento_s         = PARAM_RESTART_DEFAULT_INTERVALO_S;
    s_restart.tempo_monitoramento_pos_relig_s = PARAM_RESTART_DEFAULT_MONITOR_S;

    s_feed.feed_duration_min          = PARAM_FEED_DEFAULT_DURATION_MIN;
    s_feed.feed_cooldown_min          = PARAM_FEED_DEFAULT_COOLDOWN_MIN;

    s_security.session_timeout_min    = PARAM_SECURITY_DEFAULT_SESSION_TIMEOUT;
    s_security.max_login_attempts     = PARAM_SECURITY_DEFAULT_MAX_LOGIN;
    s_security.login_block_duration_min = PARAM_SECURITY_DEFAULT_BLOCK_DURATION;
    s_security.ack_timeout_s          = PARAM_SECURITY_DEFAULT_ACK_TIMEOUT_S;
    s_security.read_requires_auth     = true;

    s_antiflap.tempo_min_estabilizacao_s = PARAM_ANTIFLAP_DEFAULT_ESTABILIZACAO_S;
    s_antiflap.janela_flap_s             = PARAM_ANTIFLAP_DEFAULT_JANELA_S;
    s_antiflap.max_transicoes_flap       = PARAM_ANTIFLAP_DEFAULT_MAX_TRANSICOES;
    s_antiflap.cooldown_reentrada_s      = PARAM_ANTIFLAP_DEFAULT_COOLDOWN_S;

    s_selftest.selftest_timeout_ms       = PARAM_SELFTEST_DEFAULT_TIMEOUT_MS;

    s_system.wizard_completed     = false;
    s_system.mains_voltage        = PARAM_SYSTEM_DEFAULT_MAINS_VOLTAGE;
    s_system.monitor_only_mode    = PARAM_SYSTEM_DEFAULT_MONITOR_ONLY;
    s_system.maintenance_mode     = PARAM_SYSTEM_DEFAULT_MAINTENANCE_MODE;
    s_system.carousel_interval_s  = PARAM_SYSTEM_DEFAULT_CAROUSEL_INTERVAL_S;
    s_system.carousel_pause_s     = PARAM_SYSTEM_DEFAULT_CAROUSEL_PAUSE_S;

    memset(&s_calibration, 0, sizeof(s_calibration));
    for (int i = 0; i < 10; i++) {
        s_calibration.acs712_zero_offset_mv[i] = ACS712_ZERO_OFFSET_MV;
    }
    s_calibration.ato_zero_offset_adc = PARAM_CALIB_DEFAULT_ATO_ZERO_ADC;
    s_calibration.temp_offset_c = PARAM_CALIB_DEFAULT_TEMP_OFFSET_C;

    s_ph.enabled      = PARAM_PH_DEFAULT_ENABLED;
    s_ph.warn_low_ph  = PARAM_PH_DEFAULT_WARN_LOW;
    s_ph.warn_high_ph = PARAM_PH_DEFAULT_WARN_HIGH;
    s_ph.calib_min_ph = PARAM_PH_DEFAULT_CALIB_MIN;
    s_ph.calib_max_ph = PARAM_PH_DEFAULT_CALIB_MAX;
}

static esp_err_t load_nvs_blob(const char *ns, void *blob, size_t sz)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    size_t stored = sz;
    err = nvs_get_blob(h, NVS_KEY_BLOB, blob, &stored);
    nvs_close(h);
    if (err != ESP_OK || stored != sz) return ESP_ERR_NVS_NOT_FOUND;
    return ESP_OK;
}

static esp_err_t save_nvs_blob(const char *ns, const void *blob, size_t sz)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, NVS_KEY_BLOB, blob, sz);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

static esp_err_t config_store_blob(const char *ns, const void *blob, size_t sz)
{
    esp_err_t re = save_nvs_blob(ns, blob, sz);
    if (re == ESP_OK) {
        event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    }
    return re;
}

static esp_err_t schema_version_check(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS_SCHEMA, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No schema version in NVS, storing current v%s", PARAM_CATALOG_VERSION);
        return ESP_ERR_NVS_NOT_FOUND;
    }
    size_t len = sizeof(s_stored_version);
    err = nvs_get_str(h, NVS_KEY_VERSION, s_stored_version, &len);
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Schema version missing, will reset to defaults");
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (s_stored_version[0] != PARAM_CATALOG_VERSION[0]) {
        ESP_LOGW(TAG, "Schema major mismatch: stored v%s, current v%s — resetting defaults",
                 s_stored_version, PARAM_CATALOG_VERSION);
        return ESP_ERR_NVS_NOT_FOUND;
    }
    ESP_LOGI(TAG, "Schema version OK: v%s", s_stored_version);
    return ESP_OK;
}

static esp_err_t schema_version_store(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS_SCHEMA, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_str(h, NVS_KEY_VERSION, PARAM_CATALOG_VERSION);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t config_manager_init(void)
{
    set_defaults();
    memset(&s_root, 0, sizeof(s_root));
    snprintf(s_root.schema_version, sizeof(s_root.schema_version), "%s", CONFIG_ROOT_SCHEMA_VERSION);

    esp_err_t root_err = config_root_load(&s_root);
    if (root_err == ESP_OK && config_root_validate(&s_root)) {
        root_to_indiv();
        config_sanitize_system();
        ESP_LOGI(TAG, "ConfigRoot loaded from NVS (CRC=0x%08"PRIX32", wizard=%d)",
                 s_root.crc32, s_system.wizard_completed);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "ConfigRoot not found or invalid (%s), migrating individual blobs",
             esp_err_to_name(root_err));
    esp_err_t ver = schema_version_check();
    if (ver != ESP_OK) {
        ESP_LOGW(TAG, "Schema version mismatch or missing — resetting all config to defaults");
        config_reset_to_defaults();
        schema_version_store();
    }
    esp_err_t err = config_load_all();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS load failed (%s), using defaults", esp_err_to_name(err));
        config_save_all();
    }
    config_sanitize_system();
    indiv_to_root();
    config_root_save(&s_root);
    ESP_LOGI(TAG, "Config manager init OK (wizard=%d)", s_system.wizard_completed);
    return ESP_OK;
}

const thermal_params_storage_t* config_get_thermal(void) { return &s_thermal; }
const ato_params_storage_t* config_get_ato(void) { return &s_ato; }
const electric_params_storage_t* config_get_electric(void) { return &s_electric; }
const plug_limits_storage_t* config_get_plug_limits(void) { return &s_plug_limits; }
const restart_params_storage_t* config_get_restart(void) { return &s_restart; }
const feed_params_storage_t* config_get_feed(void) { return &s_feed; }
const security_params_storage_t* config_get_security(void) { return &s_security; }
const antiflap_params_storage_t* config_get_antiflap(void) { return &s_antiflap; }
const selftest_params_storage_t* config_get_selftest(void) { return &s_selftest; }
const system_params_storage_t* config_get_system(void) { return &s_system; }
const calibration_params_storage_t* config_get_calibration(void) { return &s_calibration; }
const ph_params_storage_t* config_get_ph(void) { return &s_ph; }

esp_err_t config_set_thermal(const thermal_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_thermal = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_THERMAL, &s_thermal, sizeof(s_thermal)));
}

esp_err_t config_set_ato(const ato_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_ato = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_ATO, &s_ato, sizeof(s_ato)));
}

esp_err_t config_set_electric(const electric_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_electric = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_ELECTRIC, &s_electric, sizeof(s_electric)));
}

esp_err_t config_set_plug_limits(const plug_limits_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_plug_limits = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_PLUG, &s_plug_limits, sizeof(s_plug_limits)));
}

esp_err_t config_set_restart(const restart_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_restart = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_RESTART, &s_restart, sizeof(s_restart)));
}

esp_err_t config_set_feed(const feed_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_feed = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_FEED, &s_feed, sizeof(s_feed)));
}

esp_err_t config_set_security(const security_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_security = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_SECURITY, &s_security, sizeof(s_security)));
}

esp_err_t config_set_antiflap(const antiflap_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_antiflap = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_ANTIFLAP, &s_antiflap, sizeof(s_antiflap)));
}

esp_err_t config_set_selftest(const selftest_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_selftest = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_SELFTEST, &s_selftest, sizeof(s_selftest)));
}

esp_err_t config_set_system(const system_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_system = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system)));
}

esp_err_t config_set_calibration(const calibration_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_calibration = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_CALIB, &s_calibration, sizeof(s_calibration)));
}

esp_err_t config_set_ph(const ph_params_storage_t *p)
{
    if (!p) return ESP_ERR_INVALID_ARG;
    s_ph = *p;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    return config_publish_after(config_store_blob(NVS_NS_PH, &s_ph, sizeof(s_ph)));
}

esp_err_t config_load_all(void)
{
    esp_err_t e;
    e = load_nvs_blob(NVS_NS_THERMAL, &s_thermal, sizeof(s_thermal));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_ATO, &s_ato, sizeof(s_ato));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_ELECTRIC, &s_electric, sizeof(s_electric));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_PLUG, &s_plug_limits, sizeof(s_plug_limits));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_RESTART, &s_restart, sizeof(s_restart));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_FEED, &s_feed, sizeof(s_feed));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_SECURITY, &s_security, sizeof(s_security));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_ANTIFLAP, &s_antiflap, sizeof(s_antiflap));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_SELFTEST, &s_selftest, sizeof(s_selftest));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_CALIB, &s_calibration, sizeof(s_calibration));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    e = load_nvs_blob(NVS_NS_PH, &s_ph, sizeof(s_ph));
    if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) return e;
    return ESP_OK;
}

esp_err_t config_save_all(void)
{
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    esp_err_t e;
    e = save_nvs_blob(NVS_NS_THERMAL, &s_thermal, sizeof(s_thermal));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_ATO, &s_ato, sizeof(s_ato));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_ELECTRIC, &s_electric, sizeof(s_electric));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_PLUG, &s_plug_limits, sizeof(s_plug_limits));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_RESTART, &s_restart, sizeof(s_restart));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_FEED, &s_feed, sizeof(s_feed));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_SECURITY, &s_security, sizeof(s_security));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_ANTIFLAP, &s_antiflap, sizeof(s_antiflap));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_SELFTEST, &s_selftest, sizeof(s_selftest));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_CALIB, &s_calibration, sizeof(s_calibration));
    if (e != ESP_OK) return e;
    e = save_nvs_blob(NVS_NS_PH, &s_ph, sizeof(s_ph));
    if (e != ESP_OK) return e;
    event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    return ESP_OK;
}

esp_err_t config_reset_to_defaults(void)
{
    set_defaults();
    memset(&s_root, 0, sizeof(s_root));
    snprintf(s_root.schema_version, sizeof(s_root.schema_version), "%s", CONFIG_ROOT_SCHEMA_VERSION);
    indiv_to_root();
    return config_save_all();
}

bool config_is_wizard_completed(void)
{
    return s_system.wizard_completed;
}

void config_set_wizard_completed(bool val)
{
    s_system.wizard_completed = val;
    indiv_to_root();
    config_root_save(&s_root);
    if (save_nvs_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system)) == ESP_OK) {
        event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    }
}

void config_set_monitor_only(bool val)
{
    s_system.monitor_only_mode = val;
    indiv_to_root();
    config_root_save(&s_root);
    if (save_nvs_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system)) == ESP_OK) {
        event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    }
}

uint8_t config_get_wizard_step(void)
{
    return s_system.wizard_step;
}

void config_set_wizard_step(uint8_t step)
{
    s_system.wizard_step = step;
    indiv_to_root();
    config_root_save(&s_root);
    if (save_nvs_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system)) == ESP_OK) {
        event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    }
}

uint16_t config_get_carousel_interval_s(void)
{
    return s_system.carousel_interval_s;
}

esp_err_t config_set_carousel_interval_s(uint16_t interval_s)
{
    if (!carousel_interval_valid(interval_s)) {
        return ESP_ERR_INVALID_ARG;
    }
    s_system.carousel_interval_s = interval_s;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    re = save_nvs_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system));
    if (re == ESP_OK) {
        event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    }
    return re;
}

uint16_t config_get_carousel_pause_s(void)
{
    return s_system.carousel_pause_s;
}

esp_err_t config_set_carousel_pause_s(uint16_t pause_s)
{
    if (!carousel_pause_valid(pause_s)) {
        return ESP_ERR_INVALID_ARG;
    }
    s_system.carousel_pause_s = pause_s;
    indiv_to_root();
    esp_err_t re = config_root_save(&s_root);
    if (re != ESP_OK) return re;
    re = save_nvs_blob(NVS_NS_SYSTEM, &s_system, sizeof(s_system));
    if (re == ESP_OK) {
        event_bus_publish(EVENT_ID_CONFIG_CHANGED, NULL);
    }
    return re;
}
