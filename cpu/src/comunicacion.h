#ifndef COMUNICACION_H_
#define COMUNICACION_H_

#include "main.h"
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
    int fd_dispatch;
    int fd_interrupt;
    char* server_name;
} t_procesar_conexion_args;

int server_escuchar_cpu(t_log* logger, char* server_name, int server_socket_dispatch,int server_socket_interupt);

#endif 