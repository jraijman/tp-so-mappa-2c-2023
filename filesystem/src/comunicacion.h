#ifndef COMUNICACION_H_
#define COMUNICACION_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "../../utils/src/protocolo/protocolo.h"

int conexion_filesystem_memoria;

char* ip_memoria;
char* puerto_memoria;
char* puerto_escucha;
char* path_fat;
char* path_bloques;
char* path_fcb;
char* cant_bloques_total;
char* cant_bloques_swap;
char* tam_bloque;
char* retardo_acceso_bloque;
char* retardo_acceso_fat;

typedef struct {
    t_log* log;
    int fd;
    char* server_name;
} t_procesar_conexion_args;

int server_escuchar_filesystem(t_log* logger,char* server_name,int server_socket);

#endif 