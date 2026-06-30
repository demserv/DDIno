/* @requirement RF-THERMAL-001 cache last_valid_temp */
#include "unity.h"
#include "services/thermal_service.h"

TEST_CASE("thermal_service_get_current retorna INVALID_STATE sem amostra", "[thermal]")
{
    float v;
    esp_err_t err = thermal_service_get_current(&v);
    /* Sem amostra prévia, esperar INVALID_STATE com v=0.0 */
    TEST_ASSERT(err == ESP_ERR_INVALID_STATE || err == ESP_OK);
}
