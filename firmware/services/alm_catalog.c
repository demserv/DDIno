// @requirement RF-ALERT-001/RF-ALERT-003 Tabela can├┤nica de ALMs (Tabela 9.5-A, SRS v3.11).
// Onde a SRS lista uma faixa de severidade (ex.: HIGH/CRITICAL) adota-se a mais severa
// como padr├úo; onde lista auto-clear condicional, adota-se o comportamento conservador
// (sem auto-clear para alertas que exigem ACK).
#include "alm_catalog.h"
#include "alm_ids.h"
#include <stddef.h>

#define SEV_INFO   ALERT_SEVERITY_INFO
#define SEV_WARN   ALERT_SEVERITY_WARNING
#define SEV_HIGH   ALERT_SEVERITY_HIGH
#define SEV_CRIT   ALERT_SEVERITY_CRITICAL
#define CAT_PROC   ALERT_CATEGORY_PROCESS
#define CAT_SYS    ALERT_CATEGORY_SYSTEM
#define CAT_SEC    ALERT_CATEGORY_SECURITY

/* ├ìndice = (ALM id - 1). Mant├®m a ordem ALM-001..ALM-065. */
static const alm_meta_t s_catalog[ALM_ID_MAX] = {
    /* 001 */ { SEV_INFO, CAT_SYS,  false, true,  NULL },
    /* 002 */ { SEV_INFO, CAT_SYS,  false, true,  NULL },
    /* 003 */ { SEV_HIGH, CAT_SYS,  true,  false, "Verificar travamento do firmware" },
    /* 004 */ { SEV_HIGH, CAT_SYS,  true,  false, "Verificar alimentacao" },
    /* 005 */ { SEV_HIGH, CAT_SYS,  true,  false, "Reconfigurar sistema e validar backup" },
    /* 006 */ { SEV_WARN, CAT_SYS,  false, true,  "Verificar cartao SD" },
    /* 007 */ { SEV_WARN, CAT_SYS,  false, true,  "Liberar espaco" },
    /* 008 */ { SEV_HIGH, CAT_SYS,  false, true,  "Reiniciar e investigar consumo" },
    /* 009 */ { SEV_WARN, CAT_SYS,  false, true,  "Verificar rede LAN" },
    /* 010 */ { SEV_INFO, CAT_SYS,  false, true,  NULL },
    /* 011 */ { SEV_WARN, CAT_SEC,  false, true,  "Aguardar desbloqueio ou revisar acessos" },
    /* 012 */ { SEV_INFO, CAT_SEC,  false, true,  "Efetuar novo login" },
    /* 013 */ { SEV_HIGH, CAT_PROC, true,  false, "Verificar DS18B20" },
    /* 014 */ { SEV_INFO, CAT_PROC, false, true,  NULL },
    /* 015 */ { SEV_WARN, CAT_PROC, false, true,  "Monitorar resfriamento" },
    /* 016 */ { SEV_INFO, CAT_PROC, false, true,  NULL },
    /* 017 */ { SEV_HIGH, CAT_PROC, true,  false, "Verificar sensor de nivel" },
    /* 018 */ { SEV_INFO, CAT_PROC, false, true,  NULL },
    /* 019 */ { SEV_HIGH, CAT_PROC, true,  false, "Verificar bomba e tubulacao" },
    /* 020 */ { SEV_CRIT, CAT_PROC, true,  false, "Inspecionar sistema ATO" },
    /* 021 */ { SEV_INFO, CAT_PROC, false, true,  NULL },
    /* 022 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar comunicacao PZEM" },
    /* 023 */ { SEV_INFO, CAT_PROC, false, true,  NULL },
    /* 024 */ { SEV_WARN, CAT_PROC, false, true,  "Revisar carga conectada" },
    /* 025 */ { SEV_WARN, CAT_PROC, false, true,  "Rever consumo" },
    /* 026 */ { SEV_CRIT, CAT_PROC, true,  false, "Intervencao imediata na temperatura" },
    /* 027 */ { SEV_HIGH, CAT_PROC, true,  false, "Verificar reservatorio/evaporacao" },
    /* 028 */ { SEV_CRIT, CAT_PROC, true,  false, "Intervencao imediata" },
    /* 029 */ { SEV_CRIT, CAT_SYS,  true,  false, "Verificar firmware e energia" },
    /* 030 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar fiacao/equipamento" },
    /* 031 */ { SEV_HIGH, CAT_PROC, true,  false, "Verificar carga do plugue" },
    /* 032 */ { SEV_HIGH, CAT_PROC, true,  false, "Inspecionar plugue e carga" },
    /* 033 */ { SEV_INFO, CAT_PROC, false, true,  NULL },
    /* 034 */ { SEV_INFO, CAT_PROC, false, true,  NULL },
    /* 035 */ { SEV_WARN, CAT_PROC, true,  false, "Verificar motivo do cancelamento" },
    /* 036 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar calibracao/carga" },
    /* 037 */ { SEV_CRIT, CAT_PROC, true,  false, "Verificar reposicao de agua" },
    /* 038 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar vazamento" },
    /* 039 */ { SEV_HIGH, CAT_PROC, true,  false, "Reabastecer reservatorio" },
    /* 040 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar equipamento" },
    /* 041 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar funcionamento da carga" },
    /* 042 */ { SEV_WARN, CAT_SYS,  true,  false, "Verificar/substituir SD" },
    /* 043 */ { SEV_HIGH, CAT_SYS,  true,  false, "Reiniciar e diagnosticar task" },
    /* 044 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar qualidade da rede" },
    /* 045 */ { SEV_HIGH, CAT_SYS,  true,  true,  "Usar fallback e verificar display/input" },
    /* 046 */ { SEV_CRIT, CAT_SYS,  true,  false, "Reconhecer e corrigir causa" },
    /* 047 */ { SEV_HIGH, CAT_SYS,  true,  true,  "Verificar SPI e alimentacao" },
    /* 048 */ { SEV_CRIT, CAT_SYS,  true,  false, "Verificar I2C e acionamento" },
    /* 049 */ { SEV_WARN, CAT_SYS,  true,  false, "Recalibrar sensor" },
    /* 050 */ { SEV_CRIT, CAT_PROC, true,  false, "Verificar rede eletrica" },
    /* 051 */ { SEV_CRIT, CAT_PROC, true,  false, "Verificar rede eletrica" },
    /* 052 */ { SEV_CRIT, CAT_PROC, true,  false, "Desligar cargas e inspecionar" },
    /* 053 */ { SEV_WARN, CAT_PROC, false, true,  "Verificar cargas indutivas" },
    /* 054 */ { SEV_WARN, CAT_PROC, false, true,  "Reduzir uso ou revisar limite" },
    /* 055 */ { SEV_CRIT, CAT_PROC, true,  false, "Desligar e inspecionar plugue" },
    /* 056 */ { SEV_HIGH, CAT_PROC, true,  false, "Verificar plugue e carga" },
    /* 057 */ { SEV_WARN, CAT_PROC, false, true,  "Monitorar rede" },
    /* 058 */ { SEV_WARN, CAT_PROC, false, true,  "Monitorar consumo total" },
    /* 059 */ { SEV_HIGH, CAT_SYS,  true,  true,  "Verificar subsistema afetado" },
    /* 060 */ { SEV_CRIT, CAT_SYS,  true,  false, "Revisar condicao de falha" },
    /* 061 */ { SEV_CRIT, CAT_SYS,  true,  false, "Validar NVS/backup/schema" },
    /* 062 */ { SEV_WARN, CAT_SEC,  false, true,  "Revisar tentativas de login" },
    /* 063 */ { SEV_CRIT, CAT_SYS,  true,  false, "Verificar hardware antes de operar" },
    /* 064 */ { SEV_WARN, CAT_SYS,  false, true,  "Finalizar ou revisar manutencao" },
    /* 065 */ { SEV_CRIT, CAT_PROC, true,  false, "Verificar perfil/plugue/configuracao" },
};

const alm_meta_t *alm_catalog_get(int16_t alm_id)
{
    if (alm_id < ALM_ID_MIN || alm_id > ALM_ID_MAX) {
        return NULL;
    }
    return &s_catalog[alm_id - 1];
}
