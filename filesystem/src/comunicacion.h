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

typedef struct {
    t_log* log;
    int fd;
    char* server_name;
} t_procesar_conexion_args;

int server_escuchar_filesystem(t_log* logger,char* server_name,int server_socket);

#endif 