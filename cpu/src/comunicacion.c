#include "comunicacion.h"
static void procesar_conexion(void* void_args){
   printf("entre");
    t_procesar_conexion_args* args = (t_procesar_conexion_args*)void_args;
    t_log* logger = args->log;
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    int conexion_cpu_memoria=args->conexion_cpu_memoria;
    char* server_name = args->server_name;
    free(args);
    op_code cop;
    pcb contexto;
    Direccion direccion;
    Instruccion instruccion;
    while(cliente_socket_dispatch != -1){
        switch (cop) {
        case ENVIO_PCB:{
        pcb* contexto;
        recv_pcb(cliente_socket_dispatch, &contexto);
        printf("recibi pcb con id: %d path: %s", contexto->pid, contexto->path);
        //ciclo_instruccion(contexto, cliente_socket_dispatch, cliente_socket_interrupt, logger_cpu);
        break;
      }
      case ENVIO_INSTRUCCION: { //Caso de finalizar una ejecucion
        break;
      }
      case -1: {
        log_info(logger_cpu,"El cliente se desconecto");
        break;
      }
      default: {
        log_error(logger_cpu, "No reconocido");
        break;
      }
}}
}

int server_escuchar_cpu(t_log* logger, char* server_name, int server_socket_dispatch,int server_socket_interupt, int conexion_cpu_memoria){
    int cliente_socket_dispatch = esperar_cliente(logger, server_name, server_socket_dispatch);
    int cliente_socket_interrupt = esperar_cliente(logger, server_name, server_socket_interupt);
    if (cliente_socket_dispatch != -1 /*&& cliente_socket_interrupt != -1*/) {
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

