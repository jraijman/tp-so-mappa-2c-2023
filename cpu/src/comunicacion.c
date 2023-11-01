#include "comunicacion.h"
#include <sys/socket.h>

int recibir_operacion(int socket_cliente) {
    int cod_op = -1;
    log_info(logger_cpu, "Esperando recibir una operación...");
    ssize_t bytes_recibidos = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);

    if (bytes_recibidos > 0) {
        log_info(logger_cpu, "Operación recibida correctamente. Código de operación: %d", cod_op);
        return cod_op;
    } else if (bytes_recibidos == 0) {
        log_info(logger_cpu, "La conexión fue cerrada por el otro extremo.");
        close(socket_cliente);
        return -1;
    } else {
        log_error(logger_cpu, "Error al recibir la operación:");
        log_info(logger_cpu, "Cerrando la conexión debido a un error.");
        close(socket_cliente);
        return -1;
    }
}

static void procesar_conexion(void* void_args) {
    log_info(logger_cpu, "Hilo en función procesar_conexion");
    t_procesar_conexion_args* args = (t_procesar_conexion_args*)void_args;
    t_log* logger = args->log;
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    int conexion_cpu_memoria=args->conexion_cpu_memoria;
    char* server_name = args->server_name;
    free(args);
    while(1)
    {
    uint32_t cop = recibir_operacion(cliente_socket_dispatch);
        switch (cop) {
            case ENVIO_PCB: {
                pcb contexto;
                log_info(logger_cpu,"Hay un pcb para ejecutar");
                recv_pcb(cliente_socket_dispatch, &contexto);
                if (contexto.pid!=-1) {
                    log_info(logger, "Recibí PCB con ID: %d, Path: %s", contexto.pid, contexto.path);
                    ciclo_instruccion(contexto, cliente_socket_dispatch, cliente_socket_interrupt, logger_cpu);
                } else {
                    log_error(logger, "Error al recibir el PCB");
                }
                break;
            }
            case -1:{
                			log_error(logger, "el cliente se desconecto. Terminando servidor");
                close(cliente_socket_dispatch);
                break;
            }
            default: {
                log_error(logger, "Código de operación no reconocido: %d", cop);
                break;
            }
        }
    }
    // Cerrar los sockets y liberar recursos si es necesario.
    close(cliente_socket_dispatch);
    close(cliente_socket_interrupt);
    log_info(logger_cpu, "Conexión cerrada, finalizando el procesamiento de la conexión.");
}

int server_escuchar_cpu(t_log* logger, char* server_name, int server_socket_dispatch, int server_socket_interupt, int conexion_cpu_memoria) {
    int cliente_socket_dispatch = esperar_cliente(logger, server_name, server_socket_dispatch);
    int cliente_socket_interrupt = esperar_cliente(logger, server_name, server_socket_interupt);

    if (cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd_dispatch = cliente_socket_dispatch;
        args->fd_interrupt = cliente_socket_interrupt;
        args->conexion_cpu_memoria = conexion_cpu_memoria;
        args->server_name = server_name;

        log_info(logger_cpu, "Conexión aceptada, creando hilo para procesar la conexión.");
        pthread_create(&hilo, NULL, (void*)procesar_conexion, (void*)args);
        return 1;
    } else {
        log_error(logger_cpu, "Error al esperar a los clientes");
    }

    return 0;
}