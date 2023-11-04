#ifndef COMUNICACION_H_
#define COMUNICACION_H_


#include "main.h"
#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "../../utils/src/protocolo/protocolo.h"


typedef struct {
    t_log* log;
    int fd_dispatch;
    int fd_interrupt;
} t_procesar_conexion_args;

void inicializar_estructura_proceso(int);
void manejarConexion(pcbDesalojado contexto);
int server_escuchar(t_log* logger, int server_socket_dispatch, int server_socket_interupt);
#endif 