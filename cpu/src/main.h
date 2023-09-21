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

struct ContextoEjecucion {//Modificar en el futuro segun lo que se reciba de kernel
    uint32_t pid;
    uint32_t program_counter;
    uint32_t registros[4];//Registros de uso general (AX, BX, CX, DX)
    char instrucciones[100][50]; // Array de instrucciones
};

t_log* logger_cpu;
t_config* config;

void ejecutarInstruccion(char* instr, char* arg1, char* arg2);


#endif 