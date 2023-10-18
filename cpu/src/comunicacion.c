#include "comunicacion.h"
static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*)void_args;
    t_log* logger = args->log;
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    char* server_name = args->server_name;
    free(args);

    while (cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1) {
        pcb contexto_proceso;
        ssize_t bytes_received = recv(cliente_socket_dispatch, &contexto_proceso, sizeof(pcb), 0);
        if (bytes_received <= 0) {
            log_info(logger, "Cliente desconectado de %s", server_name);
            break;
        }

        // Procesar la estructura contexto_proceso
        log_info(logger, "Se recibió un contexto de ejecución desde %s", server_name);
        log_info(logger, "PID: %u", contexto_proceso.pid);
        log_info(logger, "Program Counter: %u", contexto_proceso.pc);
        log_info(logger, "AX: %u", contexto_proceso.registros.ax);
    }

    log_warning(logger, "El cliente se desconectó de %s server", server_name);
}


void deserializar_instruccion(const void *buffer, Instruccion *instruccion) {
    memcpy(instruccion, buffer, sizeof(Instruccion));
}

int pedir_marco(int conexion_cpu_memoria, int numero_pagina)
{
    if (send(conexion_cpu_memoria, numero_pagina, sizeof(int), 0) < 0) {
        perror("Error al enviar la solicitud");
        return -1;
    }

    // Recibir la respuesta del servidor de memoria (el número de marco)
    int numero_de_marco;
    if (recv(conexion_cpu_memoria, &numero_de_marco, sizeof(int), 0) <= 0) {
        perror("Error al recibir el número de marco");
        return -1;
    }

    return numero_de_marco;
}

bool recv_instruccion(int socket_fd, Instruccion *instruccion, int *bytes_recibidos, t_log *logger) {
    char buffer[sizeof(Instruccion)];

    ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);

    if (bytes_received == -1) {
        log_error(logger, "Error al recibir la instrucción");
        *bytes_recibidos = -1;
        return false;
    } else if (bytes_received == 0) {
        log_info(logger, "Cliente desconectado");
        *bytes_recibidos = 0;
        return false;
    }

    deserializar_instruccion(buffer, instruccion);
    *bytes_recibidos = (int)bytes_received;

    return true;
}

bool send_pcb(int fd, pcb contexto) {
    size_t size = sizeof(pcb);
    if (send(fd, &contexto, size, 0) != size) {
        return false;
    }
    return true;
}
int server_escuchar_cpu(t_log* logger, char* server_name, int server_socket_dispatch,int server_socket_interupt){
     int cliente_socket_dispatch = esperar_cliente(logger, server_name, server_socket_dispatch);
     int cliente_socket_interrupt = esperar_cliente(logger, server_name, server_socket_interupt);

    if (cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd_dispatch= cliente_socket_dispatch;
        args->fd_interrupt= cliente_socket_interrupt;
        args->server_name = server_name;

        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}