// @requirement RF-WEB-004 Autenticação via token SHA-256, sessão RAM 1h
// @requirement RNF-SECURITY-001 Rate limit e bloqueio por IP
// @requirement RNF-SECURITY-002 Expiração e renovação de sessão
// @requirement RNF-SECURITY-003 Logs de auditoria de segurança
#include "api_auth.h"
#include "config_manager.h"
#include "hardware_config.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "api_auth";

#define AUTH_NVS_NS "api_auth"
#define NVS_KEY_PW_HASH "admin_pw"
#define NVS_KEY_PW_SALT "admin_salt"
#define AUTH_SALT_LEN   16
#define TOKEN_EXPIRY_MS (3600000ULL)

typedef struct {
    uint32_t ip;
    uint8_t attempts;
    uint64_t window_start_ms;
} fail_tracker_t;

static api_auth_token_t s_tokens[API_AUTH_MAX_TOKENS];
static uint8_t s_admin_hash[32];
static uint8_t s_salt[AUTH_SALT_LEN];
static bool s_has_salt = false;
static bool s_initialized = false;
static bool s_has_password = false;
    static fail_tracker_t s_fail_trackers[HW_API_AUTH_FAIL_TRACKERS];

/* @requirement RNF-SECURITY-001 Hash de senha com salt por dispositivo.
 * O salt é aleatório, persistido em NVS e prefixado à senha antes do SHA-256. */
static void hash_password(const char *password, uint8_t *out)
{
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    if (s_has_salt) {
        mbedtls_sha256_update(&ctx, s_salt, AUTH_SALT_LEN);
    }
    mbedtls_sha256_update(&ctx, (const unsigned char*)password, strlen(password));
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
}

static esp_err_t ensure_salt(nvs_handle_t nvs)
{
    if (s_has_salt) return ESP_OK;
    size_t salt_len = AUTH_SALT_LEN;
    esp_err_t err = nvs_get_blob(nvs, NVS_KEY_PW_SALT, s_salt, &salt_len);
    if (err == ESP_OK && salt_len == AUTH_SALT_LEN) {
        s_has_salt = true;
        return ESP_OK;
    }
    for (int i = 0; i < AUTH_SALT_LEN; i++) {
        s_salt[i] = (uint8_t)esp_random();
    }
    err = nvs_set_blob(nvs, NVS_KEY_PW_SALT, s_salt, AUTH_SALT_LEN);
    if (err == ESP_OK) {
        nvs_commit(nvs);
        s_has_salt = true;
    }
    return err;
}

static void to_hex(const uint8_t *in, size_t in_len, char *out)
{
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < in_len; i++) {
        out[i * 2] = hex[(in[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hex[in[i] & 0x0F];
    }
    out[in_len * 2] = '\0';
}

static void generate_token(char *out)
{
    uint8_t buf[32];
    for (int i = 0; i < 32; i++) {
        buf[i] = (uint8_t)esp_random();
    }
    to_hex(buf, 32, out);
}

/* @requirement RNF-SECURITY-002 Janela de sessão: timeout por inatividade vindo de
 * session_timeout_min (config); fallback ao limite absoluto de 1h se não configurado. */
static uint64_t session_window_ms(void)
{
    const security_params_storage_t *sec = config_get_security();
    if (sec && sec->session_timeout_min > 0) {
        return (uint64_t)sec->session_timeout_min * 60ULL * 1000ULL;
    }
    return TOKEN_EXPIRY_MS;
}

static int find_token(const char *token)
{
    if (!token) return -1;
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    for (int i = 0; i < API_AUTH_MAX_TOKENS; i++) {
        if (strlen(s_tokens[i].token) > 0 &&
            strcmp(s_tokens[i].token, token) == 0) {
            if (now_ms < s_tokens[i].expires_ms) {
                /* @requirement RNF-SECURITY-002 Sliding session: cada uso válido
                 * renova a expiração por inatividade. */
                s_tokens[i].expires_ms = now_ms + session_window_ms();
                return i;
            }
            memset(&s_tokens[i], 0, sizeof(api_auth_token_t));
            return -1;
        }
    }
    return -1;
}

static int find_token_slot(void)
{
    for (int i = 0; i < API_AUTH_MAX_TOKENS; i++) {
        if (strlen(s_tokens[i].token) == 0) {
            return i;
        }
    }
    uint64_t oldest = UINT64_MAX;
    int slot = 0;
    for (int i = 0; i < API_AUTH_MAX_TOKENS; i++) {
        if (s_tokens[i].created_ms < oldest) {
            oldest = s_tokens[i].created_ms;
            slot = i;
        }
    }
    return slot;
}

static int find_fail_entry(uint32_t ip)
{
    for (int i = 0; i < HW_API_AUTH_FAIL_TRACKERS; i++) {
        if (s_fail_trackers[i].ip == ip) return i;
    }
    for (int i = 0; i < HW_API_AUTH_FAIL_TRACKERS; i++) {
        if (s_fail_trackers[i].ip == 0) return i;
    }
    return -1;
}

static bool check_fail_limit(uint32_t ip)
{
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    int idx = find_fail_entry(ip);
    if (idx < 0) return false;

    /* @requirement RNF-SECURITY-001 Limites vindos da config (com fallback seguro):
     * max_login_attempts e janela de bloqueio login_block_duration_min. */
    const security_params_storage_t *sec = config_get_security();
    uint32_t max_attempts = (sec && sec->max_login_attempts > 0) ? sec->max_login_attempts : 5;
    uint64_t block_ms = (sec && sec->login_block_duration_min > 0)
                            ? (uint64_t)sec->login_block_duration_min * 60ULL * 1000ULL
                            : 60000ULL;

    fail_tracker_t *f = &s_fail_trackers[idx];
    if (f->ip != ip) {
        f->ip = ip;
        f->attempts = 0;
        f->window_start_ms = now_ms;
    }

    if ((now_ms - f->window_start_ms) > block_ms) {
        f->attempts = 0;
        f->window_start_ms = now_ms;
    }

    return (f->attempts < max_attempts);
}

static void record_fail(uint32_t ip)
{
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    int idx = find_fail_entry(ip);
    if (idx < 0) return;
    fail_tracker_t *f = &s_fail_trackers[idx];
    f->ip = ip;
    f->attempts++;
    f->window_start_ms = now_ms;
}

esp_err_t api_auth_init(void)
{
    if (s_initialized) return ESP_OK;

    memset(s_tokens, 0, sizeof(s_tokens));
    memset(s_fail_trackers, 0, sizeof(s_fail_trackers));

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(AUTH_NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }

    size_t salt_len = AUTH_SALT_LEN;
    if (nvs_get_blob(nvs, NVS_KEY_PW_SALT, s_salt, &salt_len) == ESP_OK && salt_len == AUTH_SALT_LEN) {
        s_has_salt = true;
    }

    size_t hash_len = 32;
    err = nvs_get_blob(nvs, NVS_KEY_PW_HASH, s_admin_hash, &hash_len);
    if (err != ESP_OK || hash_len != 32) {
        ESP_LOGW(TAG, "No admin password set in NVS — auth disabled until wizard completes");
        s_has_password = false;
    } else {
        s_has_password = true;
    }

    nvs_close(nvs);
    s_initialized = true;
    ESP_LOGI(TAG, "Auth initialized (password_set=%d)", s_has_password);
    return ESP_OK;
}

bool api_auth_has_password(void)
{
    return s_has_password;
}

esp_err_t api_auth_set_password(const char *password)
{
    if (!password || strlen(password) < 4) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(AUTH_NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    /* Garante salt por dispositivo antes de derivar o hash. */
    if (ensure_salt(nvs) != ESP_OK) {
        nvs_close(nvs);
        return ESP_FAIL;
    }

    uint8_t hash[32];
    hash_password(password, hash);

    err = nvs_set_blob(nvs, NVS_KEY_PW_HASH, hash, 32);
    if (err == ESP_OK) {
        nvs_commit(nvs);
        memcpy(s_admin_hash, hash, 32);
        s_has_password = true;
    }
    nvs_close(nvs);
    return err;
}

bool api_auth_validate(const char *token)
{
    return (find_token(token) >= 0);
}

const char* api_auth_login(const char *user, const char *password, uint32_t client_ip)
{
    uint32_t ip = client_ip;
    if (!s_initialized) return NULL;
    if (!s_has_password) return NULL;
    if (!user || !password) return NULL;
    if (strcmp(user, "admin") != 0) return NULL;

    if (!check_fail_limit(ip)) return NULL;

    uint8_t input_hash[32];
    hash_password(password, input_hash);

    if (memcmp(input_hash, s_admin_hash, 32) != 0) {
        record_fail(ip);
        return NULL;
    }

    int slot = find_token_slot();
    api_auth_token_t *t = &s_tokens[slot];
    memset(t, 0, sizeof(api_auth_token_t));
    generate_token(t->token);
    strncpy(t->user, user, API_USER_MAX - 1);
    t->created_ms = esp_timer_get_time() / USEC_PER_MSEC;
    t->expires_ms = t->created_ms + session_window_ms();

    return t->token;
}

esp_err_t api_auth_logout(const char *token)
{
    int idx = find_token(token);
    if (idx < 0) return ESP_ERR_NOT_FOUND;
    memset(&s_tokens[idx], 0, sizeof(api_auth_token_t));
    return ESP_OK;
}

int api_auth_active_count(void)
{
    int count = 0;
    uint64_t now_ms = esp_timer_get_time() / USEC_PER_MSEC;
    for (int i = 0; i < API_AUTH_MAX_TOKENS; i++) {
        if (strlen(s_tokens[i].token) > 0 && now_ms < s_tokens[i].expires_ms) {
            count++;
        }
    }
    return count;
}

esp_err_t api_auth_reset_password_to_default(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(AUTH_NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    (void)nvs_erase_key(nvs, NVS_KEY_PW_HASH);
    nvs_commit(nvs);
    nvs_close(nvs);

    memset(s_admin_hash, 0, sizeof(s_admin_hash));
    s_has_password = false;
    memset(s_tokens, 0, sizeof(s_tokens));
    ESP_LOGW(TAG, "Admin password cleared — reconfigure via wizard/API");
    return ESP_OK;
}
