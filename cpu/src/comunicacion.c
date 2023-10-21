#include "comunicacion.h"

static void procesar_conexion(void* void_args){
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
            if (recv(cliente_socket_dispatch, &cop, sizeof(cop), 0) != sizeof(cop)) {
    		log_info(logger_cpu, "DISCONNECT!");
    		return;
    	    }
            if(cop == ENVIO_PCB){
                while(cliente_socket_interrupt != -1)
                {
                if(!recv_pcb(cliente_socket_dispatch, &contexto))
                {
                    log_error(logger_cpu, "Error al recibir el PCB");
                }
                else
                {
                    log_info(logger_cpu, "Recibi PCB PID: %d, PRIORIDAD: %d", proceso.pid, proceso.prioridad);
                    sleep(1);
                    if (!fetchInstruccion(conexion_cpu_memoria, contexto, &instruccion, logger_cpu)) { 
                        //Error fetch instruccion
                    }else{   
                        if(decodeInstruccion(instruccion, &direccion) && !recv_direccion(cliente_socket_dispatch, direccion))
                        {
                            log_error(logger_cpu, "Error al obtener datos sobre la direccion")
                            //manejo de error
                        }else if (decodeInstruccion(instruccion, &direccion)){
                            uint32_t numeroPagina = direccion.direccionLogica/direccion.tamano_pagina;
                            direccion.desplazamiento = direccion.direccionLogica - (numeroPagina * direccion.tamano_pagina);
                            send_int(conexion_cpu_memoria, numeroPagina);
                            int marco;
                            if(!recv_int(cliente_socket_dispatch, &marco)){
                                //PAGE FAULT
                            }else{
                            uint32_t direccion_fisica = marco * tamanoPagina + desplazamiento;
                            log_info(logger_cpu, "Traducción de dirección lógica %s a dirección física %d", direccion_logica, direccion_fisica); 
                            //YA TENGO LA DIR FISICA ASI QUE LA GUARDO DONDE LA VAYA A USAR;
                            }
                        }
                        }
                        executeInstruccion(contexto, instruccion);
                        contexto.pc++;
                        }
                    }
                    //Si caí aca es por que tengo interrupcion.
                }
                //si cai aca no me llegó pcb
        }
        //si caí aca se desconecto
}
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
void executeInstruccion(pcb contexto_ejecucion, Instruccion instruccion) {
    log_info(logger_cpu, "Instrucción ejecutada: %s %s %s", instruccion.opcode, instruccion.operando1, instruccion.operando2);

    pcbDesalojado pcbDes;
    if (strcmp(instruccion.opcode, "SET") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0) {
            contexto_ejecucion.registros.ax = atoi(instruccion.operando2);
        }
    } else if (strcmp(instruccion.opcode, "SUM") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0 && strcmp(instruccion.operando2, "BX") == 0) {
            contexto_ejecucion.registros.ax = contexto_ejecucion.registros.ax + contexto_ejecucion.registros.bx;
        }
    } else if (strcmp(instruccion.opcode, "SUB") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0 && strcmp(instruccion.operando2, "BX") == 0) {
            contexto_ejecucion.registros.ax = contexto_ejecucion.registros.ax - contexto_ejecucion.registros.bx;
        }
    } else if (strcmp(instruccion.opcode, "SLEEP") == 0) {
        log_info(logger_cpu, "Instrucción SLEEP - Proceso se bloqueará por %s segundos", instruccion.operando1);
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "SLEEP";
        pcbDes.extra = instruccion.operando1;
        send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    } else if (strcmp(instruccion.opcode, "WAIT") == 0) {
        log_info(logger_cpu, "Instrucción WAIT - Proceso está esperando por recurso: %s", instruccion.operando1);
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "WAIT";
        pcbDes.extra = instruccion.operando1;
        send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    } else if (strcmp(instruccion.opcode, "SIGNAL") == 0) {
        log_info(logger_cpu, "Instrucción SIGNAL - Proceso ha liberado recurso: %s", instruccion.operando1);
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "SIGNAL";
        pcbDes.extra = instruccion.operando1;
        send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    } else if (strcmp(instruccion.opcode, "EXIT") == 0) {
        log_info(logger_cpu, "Instrucción EXIT - Proceso ha finalizado su ejecución");
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "EXIT";
        pcbDes.extra = "";
        send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    }
}
void traducir(Instruccion *instruccion, Direccion *direccion) {
    if(strcmp(instruccion->opcode,"MOV_OUT"))
    {
        direccion.direccionLogica=atoi(instruccion->operando1);
    }else{
        direccion.direccionLogica=atoi(instruccion->operando2);
    }
    send_direccion(conexion_cpu_memoria,direccion);
}
bool decodeInstruccion(Instruccion *instruccion, Direccion *direccion){
    if (strcmp(instruccion.opcode, "MOV_IN") == 0 || strcmp(instruccion.opcode, "F_READ") == 0 || 
    strcmp(instruccion.opcode, "F_WRITE") == 0 || strcmp(instruccion.opcode, "MOV_OUT") == 0){
        traducir(instruccion, direccion);
        return true;
    }
    log_info(logger_cpu, "Decodificando instrucción: %s %s %s", instruccion.opcode, instruccion.operando1, instruccion.operando2);
    return false;
}

bool fetchInstruccion(int fd, pcb contexto, Instruccion *instruccion, t_log* logger) {
    int bytesRecibidos;
    send_pcb(fd, &contexto); // Envía el PCB (por el PID y PC para que te envíen la instrucción correspondiente)
    if (recv_instruccion(fd, instruccion)) {
        log_info(logger, "Instrucción recibida: %s %s %s", instruccion->opcode, instruccion->operando1, instruccion->operando2);
        log_info(logger, "PID: %d - FETCH - Program Counter: %d", contexto.pid, contexto.pc);
        return true;
    } else {
        log_error(logger, "Error al recibir la instrucción");
        return false;
    }
}
