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
typedef struct {
    char opcode[11];   // Código de operación (por ejemplo, "SUM", "SUB", "SET", "EXIT")
    char operando1[25];
    char operando2[25];
} Instruccion;

int conexion_memoria_filesystem;

char* puerto_escucha;
char* ip_filesystem;
char* puerto_filesystem;
char* tam_memoria;
char* tam_pagina;
char* path_instrucciones;
char* retardo_respuesta;
char* algoritmo_reemplazo;

t_log* logger_memoria;
t_config* config;

#endif 