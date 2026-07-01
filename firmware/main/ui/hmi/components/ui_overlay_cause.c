// @requirement RF-UI-OVERLAY-001.1 Instruções por causa (12 templates normativos)
#include "ui_overlay_cause.h"
#include <stddef.h>

static const ui_overlay_cause_template_t s_templates[] = {
    [SAFEOFF_REASON_THERMAL_CRITICAL] = {
        .title = "SAFE_OFF: TEMPERATURA CRITICA",
        .occurred = "Temperatura acima do limite critico.",
        .impact = "Cargas nao-criticas desligadas; aquecimento/resfriamento bloqueado.",
        .action = "Resfrie o aquario; verifique termostato e circulacao.",
        .exit_hint = "Saida: temp < limite critico + ACK se exigido."
    },
    [SAFEOFF_REASON_THERMAL_EXTREME] = {
        .title = "EMERGENCY: TEMPERATURA EXTREMA",
        .occurred = "Temperatura extrema detectada.",
        .impact = "Todas as cargas desligadas; risco biologico imediato.",
        .action = "Intervencao manual urgente; verifique sensores DS18B20.",
        .exit_hint = "Saida: temp normalizada + reset manual."
    },
    [SAFEOFF_REASON_ATO_OVERFLOW] = {
        .title = "SAFE_OFF: ATO OVERFLOW",
        .occurred = "Nivel ATO acima do limite de overflow.",
        .impact = "Bomba ATO bloqueada; risco de transbordamento.",
        .action = "Verifique sensor digital ATO e valvula de enchimento.",
        .exit_hint = "Saida: nivel normalizado + desbloqueio ATO."
    },
    [SAFEOFF_REASON_ELECTRIC_TOTAL] = {
        .title = "SAFE_OFF: POTENCIA TOTAL",
        .occurred = "Potencia total acima do limite configurado.",
        .impact = "Religamento sequencial bloqueado.",
        .action = "Reduza cargas; revise limite em Config > Eletrico.",
        .exit_hint = "Saida: potencia abaixo do limite."
    },
    [SAFEOFF_REASON_PLUG_SHORT] = {
        .title = "SAFE_OFF: CURTO EM PLUGUE",
        .occurred = "Corrente de curto-circuito detectada em plugue.",
        .impact = "Plugue afetado bloqueado e desenergizado.",
        .action = "Desconecte carga; inspecione fiação do plugue.",
        .exit_hint = "Saida: corrente normal + desbloqueio do plugue."
    },
    [SAFEOFF_REASON_MCP23017_FAIL] = {
        .title = "SAFE_OFF: FALHA I/O",
        .occurred = "Falha de comunicacao com expansor MCP23017.",
        .impact = "Controle de reles indisponivel.",
        .action = "Verifique barramento I2C e alimentacao 5V.",
        .exit_hint = "Saida: I/O restaurado + self-test OK."
    },
    [SAFEOFF_REASON_CONFIG_INVALID] = {
        .title = "SAFE_OFF: CONFIG INVALIDA",
        .occurred = "Configuracao corrompida ou CRC invalido.",
        .impact = "Operacao automatica suspensa.",
        .action = "Restaure perfil ou faca reset de fabrica.",
        .exit_hint = "Saida: import valido ou wizard concluido."
    },
    [SAFEOFF_REASON_SELFTEST_FAIL] = {
        .title = "SAFE_OFF: SELF-TEST FALHOU",
        .occurred = "Autoteste de hardware falhou na inicializacao.",
        .impact = "Sistema em modo protegido.",
        .action = "Consulte Diagnosticos; verifique sensores e SD.",
        .exit_hint = "Saida: self-test passa apos correcao."
    },
    [SAFEOFF_REASON_FSM_INVALID] = {
        .title = "SAFE_OFF: FSM INVALIDA",
        .occurred = "Estado interno de maquina de estados inconsistente.",
        .impact = "Comandos de carga bloqueados.",
        .action = "Reinicie o controlador; reporte se persistir.",
        .exit_hint = "Saida: reboot + FSM valida."
    },
    [SAFEOFF_REASON_WDT_RECOVERY] = {
        .title = "SAFE_OFF: RECUPERACAO WDT",
        .occurred = "Watchdog reiniciou tarefa critica.",
        .impact = "Estado seguro forcado pos-recuperacao.",
        .action = "Verifique logs; confirme estabilidade do firmware.",
        .exit_hint = "Saida: ACK + condicoes normais."
    },
    [SAFEOFF_REASON_MANUAL_CRITICAL] = {
        .title = "SAFE_OFF: MANUAL",
        .occurred = "Operador acionou SAFE_OFF manualmente.",
        .impact = "Cargas desligadas conforme politica.",
        .action = "Confirme condicoes antes de religar.",
        .exit_hint = "Saida: comando de restauracao autorizado."
    },
    [SAFEOFF_REASON_OVERVOLTAGE] = {
        .title = "SAFE_OFF: SOBRETENSAO",
        .occurred = "Tensao de rede acima do limite.",
        .impact = "Todas as cargas desenergizadas.",
        .action = "Aguarde estabilizacao da rede eletrica.",
        .exit_hint = "Saida: tensao dentro da faixa por tempo configurado."
    },
    [SAFEOFF_REASON_UNDERVOLTAGE] = {
        .title = "SAFE_OFF: SUBTENSAO",
        .occurred = "Tensao de rede abaixo do limite.",
        .impact = "Todas as cargas desenergizadas.",
        .action = "Verifique alimentacao e disjuntores.",
        .exit_hint = "Saida: tensao dentro da faixa por tempo configurado."
    }
};

static const ui_overlay_cause_template_t s_default = {
    .title = "SAFE_OFF ATIVO",
    .occurred = "Condicao critica detectada pelo sistema.",
    .impact = "Cargas criticas desligadas conforme politica.",
    .action = "Verifique alertas ativos e diagnostico.",
    .exit_hint = "Saida: causa resolvida + ACK se exigido."
};

const ui_overlay_cause_template_t *ui_overlay_cause_lookup(safeoff_reason_t reason)
{
    if (reason <= 0 || reason >= (safeoff_reason_t)(sizeof(s_templates) / sizeof(s_templates[0]))) {
        return &s_default;
    }
    if (s_templates[reason].title == NULL) {
        return &s_default;
    }
    return &s_templates[reason];
}
