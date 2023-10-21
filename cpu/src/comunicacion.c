#include "comunicacion.h"
static void procesar_conexion(void* void_args){
    t_procesar_conexion_args* args = (t_procesar_conexion_args*)void_args;
    t_log* logger = args->log;
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    char* server_name = args->server_name;
    free(args);

    op_code cop;
    while(cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1){
        if (recv(cliente_socket_dispatch, &cop, sizeof(cop), 0) != sizeof(cop) || recv(cliente_socket_interrupt, &cop, sizeof(cop), 0) != sizeof(cop)) {
    		log_info(logger, "DISCONNECT!");
    		return;
    	}
        switch (cop) {
            case ENVIO_PCB:
            {
                pcb proceso;
                if (!recv_pcb(cliente_socket_dispatch, &proceso)) {
                    log_error(logger, "Fallo recibiendo ENVIO_PCB");
                    break;
                }
                log_info(logger, "recibi pcb id: %d, prioridad: %d", proceso.pid, proceso.prioridad);
                break;
            }
            case ENVIO_INSTRUCCION:
            {
                Instruccion instruccion;
                if (!recv_instruccion(cliente_socket_dispatch, &instruccion)) {
                    log_error(logger, "Fallo recibiendo instruccion");
                    break;
                }
                log_info(logger, "Recibi la instruccion codOp: %s, operando1: %s, operando2: %s", instruccion.opcode , instruccion.operando1, instruccion.operando1);
                break;
            }
            // Errores
            case -1:
                log_error(logger, "Cliente desconectado de %s...", server_name);
                return;
            default:
                log_error(logger, "Algo anduvo mal en el server de %s", server_name);
                return;
        }

    }

    log_warning(logger, "El cliente se desconecto de %s server", server_name);
    return;
}

int pedir_marco(int conexion_cpu_memoria, int numero_pagina)
{
    if (send(conexion_cpu_memoria, &numero_pagina, sizeof(int), 0) < 0) {
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