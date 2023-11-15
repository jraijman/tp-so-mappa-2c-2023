#include "main.h"

int main(int argc, char* argv[]){
    // CONFIG y logger
    levantar_config("cpu.config");
    logger_cpu = iniciar_logger("cpu.log", "CPU:");

    // Inicio servidor de dispatch e interrupt para el kernel
    fd_cpu_dispatch = iniciar_servidor(logger_cpu, NULL, puerto_dispatch, "CPU DISPATCH");
    fd_cpu_interrupt = iniciar_servidor(logger_cpu, NULL, puerto_interrupt, "CPU INTERRUPT");

    // Genero conexión a memoria
    conexion_cpu_memoria = crear_conexion(logger_cpu, "MEMORIA", ip_memoria, puerto_memoria);  
    //mensaje prueba 
    enviar_mensaje("Hola, soy CPU!", conexion_cpu_memoria);
    
    // Espero msjs
    while(server_escuchar(logger_cpu, fd_cpu_interrupt, fd_cpu_dispatch));

    // Cerrar LOG y CONFIG y liberar conexión
    terminar_programa(logger_cpu, config);
    liberar_conexion(conexion_cpu_memoria);
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
        pthread_create(&hilo_cpu, NULL,(void*)procesar_conexion, args);
        return 1;
 	}
    else{   
        log_info(logger, "Hubo un error en la conexion del Kernel");
    }
    return 0;
}

static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*)void_args;
    t_log* logger = args->log;
    log_info(logger, ANSI_COLOR_BLUE "Hilo en función procesar_conexion");
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    free(args);
    while(1)
    {
        int cop = recibir_operacion(cliente_socket_dispatch);
        log_info(logger, "%d", cop);
        switch (cop) {
            case MENSAJE:
                recibir_mensaje(logger, cliente_socket_dispatch);
                break;
		    case PAQUETE:
                t_list *paquete_recibido = recibir_paquete(cliente_socket_dispatch);
                log_info(logger, ANSI_COLOR_YELLOW "Recibí un paquete con los siguientes valores: ");
                break;
            case ENVIO_PCB: {
                pcb* contexto=recv_pcb(cliente_socket_dispatch);
                if (contexto->pid!=-1) {
                    log_info(logger, ANSI_COLOR_YELLOW "Recibí PCB con ID: %d", contexto->pid);
                    enviar_mensaje("deberia mandar pcb desalojado", cliente_socket_dispatch);
                    sleep(0.99);
                    ciclo_instruccion(contexto, cliente_socket_dispatch, cliente_socket_interrupt, logger);
                    return;
                } else {
                    log_error(logger, "Error al recibir el PCB");
                    return;
                }
                break;
            }
            case -1:{
                			log_error(logger, "el cliente se desconecto. Terminando servidor");
                            close(cliente_socket_dispatch);
                            return;
                break;
            }
            default: {
                log_error(logger, "Código de operación no reconocido: %d", cop);
                break;
            }
        }
    }
    // Cerrar los sockets y liberar recursos si es necesario.
    //free(args);
    close(cliente_socket_dispatch);
    close(cliente_socket_interrupt);
    log_info(logger_cpu, ANSI_COLOR_BLUE "Conexión cerrada, finalizando el procesamiento de la conexión.");
}

void levantar_config(char* ruta){
    logger_cpu = iniciar_logger("cpu.log", "CPU:");
    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
}

void ciclo_instruccion(pcb* contexto, int cliente_socket_dispatch, int cliente_socket_interrupt, t_log* logger) {
    log_info(logger, "Inicio del ciclo de instrucción");
    while (cliente_socket_dispatch != -1) {
        while (cliente_socket_interrupt != -1) {
            Instruccion instruccion;
            fetchInstruccion(conexion_cpu_memoria, contexto, &instruccion, logger);
            contexto->pc++;
            log_info(logger, "Instrucción recibida");
            decodeInstruccion(&instruccion,contexto);
            executeInstruccion(contexto, instruccion);
        }
        send_pcbDesalojado(contexto,"INTERRUPCION","",cliente_socket_dispatch, logger);
        return;
    }
}

void executeInstruccion(pcb* contexto_ejecucion, Instruccion instruccion) {
    log_info(logger_cpu, "Instrucción ejecutada: %s %s %s", instruccion.opcode, instruccion.operando1, instruccion.operando2);

    if (strcmp(instruccion.opcode, "SET") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0) {
            *(contexto_ejecucion->registros->ax) = atoi(instruccion.operando2);
        }
    } else if (strcmp(instruccion.opcode, "SUM") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0 && strcmp(instruccion.operando2, "BX") == 0) {
            *(contexto_ejecucion->registros->ax) = *(contexto_ejecucion->registros->ax) + *(contexto_ejecucion->registros->bx);
        }
    } else if (strcmp(instruccion.opcode, "SUB") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0 && strcmp(instruccion.operando2, "BX") == 0) {
            *(contexto_ejecucion->registros->ax) = *(contexto_ejecucion->registros->ax) - *(contexto_ejecucion->registros->bx);
        }
    } else if (strcmp(instruccion.opcode, "SLEEP") == 0) {
        log_info(logger_cpu, "Instrucción SLEEP - Proceso se bloqueará por %s segundos", instruccion.operando1);
        send_pcbDesalojado(contexto_ejecucion,"SLEEP",instruccion.operando1,fd_cpu_dispatch,logger_cpu); 

    } else if (strcmp(instruccion.opcode, "WAIT") == 0) {
        log_info(logger_cpu, "Instrucción WAIT - Proceso está esperando por recurso: %s", instruccion.operando1);
        send_pcbDesalojado(contexto_ejecucion,"WAIT",instruccion.operando1,fd_cpu_dispatch,logger_cpu);
    } else if (strcmp(instruccion.opcode, "SIGNAL") == 0) {
        log_info(logger_cpu, "Instrucción SIGNAL - Proceso ha liberado recurso: %s", instruccion.operando1);
        send_pcbDesalojado(contexto_ejecucion,"SIGNAL",instruccion.operando1,fd_cpu_dispatch,logger_cpu);
    } else if (strcmp(instruccion.opcode, "EXIT") == 0) {
        log_info(logger_cpu, "Instrucción EXIT - Proceso ha finalizado su ejecución");
        send_pcbDesalojado(contexto_ejecucion,"EXIT","",fd_cpu_dispatch,logger_cpu);
    }
}

void decodeInstruccion(Instruccion *instruccion, pcb* contexto){
    log_info(logger_cpu, "Decoding instruccion");
    Direccion direccion;
    if (strcmp(instruccion->opcode, "MOV_IN") == 0 || strcmp(instruccion->opcode, "F_READ") == 0 || 
    strcmp(instruccion->opcode, "F_WRITE") == 0 || strcmp(instruccion->opcode, "MOV_OUT") == 0){
        traducir(instruccion, &direccion, contexto);
    }
    log_info(logger_cpu, "Decodificando instrucción: %s %s %s", instruccion->opcode, instruccion->operando1, instruccion->operando2);
}

bool fetchInstruccion(int fd, pcb* contexto, Instruccion *instruccion, t_log* logger) {
    log_info (logger, "Fetch de instruccion");
    Instruccion aux;
    send_fetch_instruccion(contexto->pid,contexto->pc, fd); // Envía paquete para pedir instrucciones
    aux=recv_instruccion(fd);
    strcpy(instruccion->opcode, aux.opcode);
    strcpy(instruccion->operando1, aux.operando1);
    strcpy(instruccion->operando2, aux.operando2);
    if (instruccion!=NULL) {
        log_info(logger, "Instrucción recibida: %s %s %s", instruccion->opcode, instruccion->operando1, instruccion->operando2);
        log_info(logger, "PID: %d - FETCH - Program Counter: %d", contexto->pid, contexto->pc);
        return true;
    } else {
        log_error(logger, "Error al recibir la instrucción");
        return false;
    }
}