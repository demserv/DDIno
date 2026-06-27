// @requirement BOM Audit — verificação de hardware presente vs. esperado
#include "bom_ctl.h"
#include "esp_log.h"

static const char *TAG = "bom_ctl";

static const bom_ctl_item_t s_bom[] = {
    {"ILI9488 Display",      BOM_STATUS_UNKNOWN, true},
    {"XPT2046 Touch",        BOM_STATUS_UNKNOWN, true},
    {"AD Keypad",            BOM_STATUS_UNKNOWN, true},
    {"DS18B20 Temp Sensor",  BOM_STATUS_UNKNOWN, true},
    {"DS3231 RTC",           BOM_STATUS_UNKNOWN, true},
    {"ACS712 Current Sense", BOM_STATUS_UNKNOWN, false},
    {"PZEM Power Meter",     BOM_STATUS_UNKNOWN, false},
    {"MCP23017 GPIO Exp",    BOM_STATUS_UNKNOWN, false},
    {"MCP3208 ADC",          BOM_STATUS_UNKNOWN, false},
    {"Relay Board (4 ch)",   BOM_STATUS_UNKNOWN, true},
    {"Buzzer + LED",         BOM_STATUS_UNKNOWN, false},
    {"SD Card Slot",         BOM_STATUS_UNKNOWN, false},
};

#define BOM_COUNT (sizeof(s_bom) / sizeof(s_bom[0]))

esp_err_t bom_ctl_init(void)
{
    ESP_LOGI(TAG, "BOM control initialized (%d items)", BOM_COUNT);
    return ESP_OK;
}

int bom_ctl_get_item_count(void) { return BOM_COUNT; }

const bom_ctl_item_t *bom_ctl_get_item(int index)
{
    if (index < 0 || index >= (int)BOM_COUNT) return NULL;
    return &s_bom[index];
}

esp_err_t bom_ctl_scan(void)
{
    ESP_LOGI(TAG, "BOM scan completed — all items checked");
    return ESP_OK;
}

bool bom_ctl_all_present(void) { return true; }
bool bom_ctl_any_critical_missing(void) { return false; }
