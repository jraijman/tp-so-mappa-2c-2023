#ifndef COMUNICACION_H_
#define COMUNICACION_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "main.h"
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "../../utils/src/protocolo/protocolo.h"


typedef struct Proceso{
    int pid; // Identificador del proceso
    int estado; // Estado del proceso (NEW, READY, EXEC, BLOCK, EXIT,ETC)
    char* rutaArchivo; // Ruta del archivo del proceso
    struct Proceso* siguiente; // Puntero al siguiente proceso en la lista
}Proceso;


typedef struct {
    t_log* log;
    int fd;
    char* server_name;
} t_procesar_conexion_args;

int server_escuchar_memoria(t_log* logger,char* server_name,int server_socket);
void inicializar_estructura_proceso(int pid);
bool notificar_liberacion_swap(int socket_fd, int pid, int cantidad_bloques, int* bloques);
//void manejarConexion(pcbDesalojado contexto);

#endif 