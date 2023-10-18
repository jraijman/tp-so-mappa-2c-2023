#ifndef MAIN_H_
#define MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "comunicacion.h"

int  conexion_cpu_memoria;
char* ip_memoria;
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
typedef struct {
    char opcode[11];   // Código de operación (por ejemplo, "SUM", "SUB", "SET", "EXIT")
    char operando1[25];
    char operando2[25];
} Instruccion;

t_log* logger_cpu;
t_config* config;

void ejecutarInstruccion(ContextoEjecucion*,char* instr, char* arg1, char* arg2);


#endif 