#include "comunicacion.h"
#include <sys/socket.h>

static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*)void_args;
    t_log* logger = args->log;
    log_info(logger, "Hilo en función procesar_conexion");
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    while(1)
    {
        int cop = recibir_operacion(cliente_socket_dispatch);
        log_info(logger, "%d", cop);
        switch (cop) {
            case ENVIO_PCB: {
                pcb* contexto=recv_pcb(cliente_socket_dispatch);
                log_info(logger_cpu,"Hay un pcb para ejecutar");
                if (contexto->pid!=-1) {
                    log_info(logger, "Recibí PCB con ID: %d", contexto->pid);
                    enviar_mensaje("deberia mandar pcb desalojad", cliente_socket_interrupt);
                    //ciclo_instruccion(contexto, cliente_socket_dispatch, cliente_socket_interrupt, logger);
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
    free(args);
    close(cliente_socket_dispatch);
    close(cliente_socket_interrupt);
    log_info(logger_cpu, "Conexión cerrada, finalizando el procesamiento de la conexión.");
}

int server_escuchar(t_log* logger, int fd_cpu_interrupt, int fd_cpu_dispatch) {

	char* server_name = "CPU";
	int socket_cliente_interrupt = esperar_cliente(logger, server_name, fd_cpu_interrupt);
    int socket_cliente_dispatch = esperar_cliente(logger, server_name, fd_cpu_dispatch);
	
    if (socket_cliente_interrupt != -1 && socket_cliente_dispatch !=-1) {
        pthread_t hilo_cpu;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd_dispatch = socket_cliente_dispatch;
        args->fd_interrupt = socket_cliente_interrupt;
        log_info(logger_cpu, "Conexión aceptada, creando hilo para procesar la conexión.");
        pthread_create(&hilo_cpu, NULL,(void*)procesar_conexion, args);
        return 1;
 	}
    else{
        
        log_info(logger, "Hubo un error en la conexion del Kernel");
    }
    return 0;
}
