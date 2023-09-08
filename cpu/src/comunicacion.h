#ifndef COMUNICACION_H_
#define COMUNICACION_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <utils/utils.h>
#include <sockets/sockets.h>
#include <protocolo/protocolo.h>

typedef struct {
    t_log* log;
    int fd_dispatch;
    int fd_interrupt;
    char* server_name;
} t_procesar_conexion_args;

int server_escuchar_cpu(t_log* logger, char* server_name, int server_socket_dispatch,int server_socket_interupt);


#endif 