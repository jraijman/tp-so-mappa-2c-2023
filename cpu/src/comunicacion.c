#include "comunicacion.h"

int server_escuchar_cpu(t_log* logger, char* server_name, int server_socket_dispatch,int server_socket_interupt, int conexion_cpu_memoria){
    int cliente_socket_dispatch = esperar_cliente(logger, server_name, server_socket_dispatch);
    int cliente_socket_interrupt = esperar_cliente(logger, server_name, server_socket_interupt);
    
    if (cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd_dispatch= cliente_socket_dispatch;
        args->fd_interrupt= cliente_socket_interrupt;
        args->conexion_cpu_memoria=conexion_cpu_memoria;
        args->server_name = server_name;

        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}

