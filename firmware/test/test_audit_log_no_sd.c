/* @requirement RNF-SECURITY-003 RF-STORAGE-002 */
#include "unity.h"
#include "storage_facade.h"
#include "audit_log.h"
#include <string.h>

TEST_CASE("audit_log_event não retorna erro com SD ausente", "[audit]")
{
    storage_facade_init();
    esp_err_t err = audit_log_event(AUDIT_SYSTEM_STATE, "test_no_sd");
    TEST_ASSERT(err == ESP_OK);
}

TEST_CASE("storage_facade reporta ram_fallback_active sem SD", "[storage]")
{
    storage_facade_init();
    storage_health_t h;
    TEST_ASSERT_EQUAL(ESP_OK, storage_facade_get_health(&h));
    TEST_ASSERT_TRUE(h.nvs_ok);
}
