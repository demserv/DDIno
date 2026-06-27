// @requirement RF-GLOBAL-004 Mapeamento de causas de SAFE_OFF para ALMs
#include "safeoff_alm_map.h"
#include "alm_ids.h"

int16_t safeoff_reason_to_alm_id(safeoff_reason_t reason)
{
    switch (reason) {
        case SAFEOFF_REASON_NONE:             return -1;
        case SAFEOFF_REASON_THERMAL_CRITICAL: return ALM_026;
        case SAFEOFF_REASON_THERMAL_EXTREME:  return ALM_028;
        case SAFEOFF_REASON_ATO_OVERFLOW:     return ALM_037;
        case SAFEOFF_REASON_ELECTRIC_TOTAL:   return ALM_052;
        case SAFEOFF_REASON_OVERVOLTAGE:      return ALM_052;
        case SAFEOFF_REASON_UNDERVOLTAGE:     return ALM_052;
        case SAFEOFF_REASON_PLUG_SHORT:       return ALM_055;
        case SAFEOFF_REASON_MCP23017_FAIL:    return ALM_048;
        case SAFEOFF_REASON_CONFIG_INVALID:   return ALM_061;
        case SAFEOFF_REASON_SELFTEST_FAIL:    return ALM_063;
        case SAFEOFF_REASON_FSM_INVALID:      return ALM_060;
        case SAFEOFF_REASON_WDT_RECOVERY:     return ALM_003;
        case SAFEOFF_REASON_MANUAL_CRITICAL:  return ALM_065;
        default:                              return -1;
    }
}
