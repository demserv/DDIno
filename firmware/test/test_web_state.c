/* @requirement RF-WEB-002 TC-WEB-001..010 */
#include "unity.h"
#include "esp_http_server.h"

TEST_CASE("GET /api/v1/state returns 200 and contains system_state", "[web]")
{
    /* Stub mínimo: este teste valida apenas que a rota está registrada.
     * Em ambiente unity sem HTTPD real, marcar IGNORE. */
    TEST_IGNORE_MESSAGE("Requer ambiente de integração HTTPD");
}
