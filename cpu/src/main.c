#include "main.h"

int main(int argc, char* argv[]){

    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }
    
    // CONFIG y logger
    levantar_config(argv[1]);


    // Genero conexión a memoria
    conexion_cpu_memoria = crear_conexion(logger_cpu, "MEMORIA", ip_memoria, puerto_memoria);  
    //mensaje prueba 
    enviar_mensaje("Hola, soy CPU!", conexion_cpu_memoria);

    // Inicio servidor de dispatch e interrupt para el kernel
    fd_cpu_dispatch = iniciar_servidor(logger_cpu, NULL, puerto_dispatch, "CPU DISPATCH");
    fd_cpu_interrupt = iniciar_servidor(logger_cpu, NULL, puerto_interrupt, "CPU INTERRUPT");

    //pido tamaño de pagina y recibo
    //HACER send_tam_pagina y recv_tam_pagina
    enviar_mensaje("TAM_PAGINA", conexion_cpu_memoria);

    op_code cop = recibir_operacion(conexion_cpu_memoria);
    tamPaginaGlobal = recv_tam_pagina(conexion_cpu_memoria);
    log_info(logger_cpu, "Tamaño de página: %d", tamPaginaGlobal);

    
    // Espero msjs
    while(server_escuchar(fd_cpu_interrupt, fd_cpu_dispatch));

    // Cerrar LOG y CONFIG y liberar conexión
    terminar_programa(logger_cpu, config);
    liberar_conexion(conexion_cpu_memoria);
}

static void procesar_conexion_interrupt(void* void_args) {
    int *args = (int*) void_args;
	int cliente_socket_interrupt = *args;

    op_code cop;
	while (cliente_socket_interrupt != -1) {
		if (recv(cliente_socket_interrupt, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger_cpu, ANSI_COLOR_BLUE"El cliente se desconecto de INTERRUPT");
			return;
		}
        switch (cop) {
            case MENSAJE:
                recibir_mensaje(logger_cpu, cliente_socket_interrupt);
                break;
		    case PAQUETE:
                t_list *paquete_recibido = recibir_paquete(cliente_socket_interrupt);
                log_info(logger_cpu, ANSI_COLOR_YELLOW "Recibí un paquete con los siguientes valores: ");
                break;
            case INTERRUPCION:
                int pid_recibido = recv_interrupcion(cliente_socket_interrupt);
                log_info(logger_cpu, ANSI_COLOR_YELLOW "Recibí una interrupcion al proceso %d, mientras ejecutaba el proceso %d",pid_recibido,contexto->pid);
                if(contexto->pid==pid_recibido){
                recibio_interrupcion = true;
                } 
                break;
            case INTERRUPCION_FINALIZAR:
                int pid_recibido_finalizar = recv_interrupcion(cliente_socket_interrupt);
                log_info(logger_cpu, ANSI_COLOR_YELLOW "se finaliza el proceso %d, mientras ejecutaba el proceso %d",pid_recibido,contexto->pid);
                if(contexto->pid==pid_recibido_finalizar){
                recibio_interrupcion = true;
                }
                break;
            default: {
                log_error(logger_cpu, "Código de operación no reconocido en Interrupt: %d", cop);
                break;
            }
        }
    }
}

static void procesar_conexion_dispatch(void* void_args) {
    int *args = (int*) void_args;
	int cliente_socket_dispatch = *args;

    op_code cop;
	while (cliente_socket_dispatch != -1 && !recibio_interrupcion) {
        if (recv(cliente_socket_dispatch, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger_cpu, ANSI_COLOR_BLUE"El cliente se desconecto de DISPATCH");
			return;
		}
        switch (cop) {
            case MENSAJE:
                recibir_mensaje(logger_cpu, cliente_socket_dispatch);
                break;
		    case PAQUETE:
                t_list *paquete_recibido = recibir_paquete(cliente_socket_dispatch);
                //log_info(logger_cpu, ANSI_COLOR_YELLOW "Recibí un paquete con los siguientes valores: ");
                break;
            case ENVIO_PCB: 
                flag_ciclo = true;
                contexto = recv_pcb(cliente_socket_dispatch);
                if (contexto->pid!=-1) {
                    //log_info(logger_cpu, ANSI_COLOR_YELLOW "Recibí PCB con ID: %d", contexto->pid);
                    ciclo_instruccion(contexto, cliente_socket_dispatch,cliente_socket_dispatch, logger_cpu);
                    pcb_destroyer(contexto);
                    break;    
                } else {
                    log_error(logger_cpu, "Error al recibir el PCB");
                }
                break;
            default: {
                log_error(logger_cpu, "Código de operación no reconocido en Dispatch: %d", cop);
                break;
            }
        }
    }
    return;
}

int server_escuchar(int fd_cpu_interrupt, int fd_cpu_dispatch) {
	int socket_cliente_interrupt = esperar_cliente(logger_cpu, "CPU INTERRUPT", fd_cpu_interrupt);
    char * server_name = "CPU DISPATCH";
    int socket_cliente_dispatch = esperar_cliente(logger_cpu, server_name, fd_cpu_dispatch);
	
    if (socket_cliente_interrupt != -1 && socket_cliente_dispatch != -1) {
        //hilo para servidor dispatch
        pthread_t hilo_dispatch;
		pthread_create(&hilo_dispatch, NULL, (void*) procesar_conexion_dispatch, (void*) &socket_cliente_dispatch);
		pthread_detach(hilo_dispatch);
        //hilo para servidor interrupt
        pthread_t hilo_interrupt;
		pthread_create(&hilo_interrupt, NULL, (void*) procesar_conexion_interrupt, (void*) &socket_cliente_interrupt);
		pthread_detach(hilo_interrupt);
        return 1;
 	}
    return 0;
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
    //log_info(logger,ANSI_COLOR_BLUE "Inicio del ciclo de instrucción");
    while (cliente_socket_dispatch != -1 && !recibio_interrupcion && flag_ciclo) {
    Instruccion * instruccion = malloc(sizeof(Instruccion));
    //int direccionFisica;    
    DireccionFisica direccion;
        if(fetchInstruccion(conexion_cpu_memoria, contexto, instruccion, logger)){
            direccion=decodeInstruccion(instruccion,contexto,cliente_socket_dispatch);
            if(direccion.marco >= 0 || direccion.marco == -2){
            contexto->pc++;
            executeInstruccion(contexto, *instruccion, direccion, cliente_socket_dispatch, conexion_cpu_memoria);
            }else{
                free(instruccion->opcode);
                free(instruccion->operando1);
                free(instruccion->operando2);
                free(instruccion);
                return;
            }
        }
        free(instruccion->opcode);
        free(instruccion->operando1);
        free(instruccion->operando2);
        free(instruccion);
    }
    if(recibio_interrupcion){
        recibio_interrupcion=false;
        send_pcbDesalojado(contexto,"INTERRUPCION","",cliente_socket_dispatch, logger);
        return;
    }
}

void executeInstruccion(pcb* contexto_ejecucion, Instruccion instruccion,DireccionFisica direccion ,int fd_dispatch, int fd_memoria) {
    log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s",contexto_ejecucion->pid, instruccion.opcode, instruccion.operando1, instruccion.operando2);

    if (strcmp(instruccion.opcode, "SET") == 0) {
        setInstruccion(contexto_ejecucion, instruccion, logger_cpu);
    } else if (strcmp(instruccion.opcode, "SUM") == 0) {
        sumInstruccion(contexto_ejecucion, instruccion, logger_cpu);
    } else if (strcmp(instruccion.opcode, "SUB") == 0) {
        subInstruccion(contexto_ejecucion, instruccion, logger_cpu);
    } else if (strcmp(instruccion.opcode, "SLEEP") == 0) {
        flag_ciclo = false;
        sleepInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode, "WAIT") == 0) {
        flag_ciclo = false;
        waitInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode, "SIGNAL") == 0) {
        flag_ciclo = false;
        signalInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode, "EXIT") == 0) {
        flag_ciclo = false;
        exitInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode,"MOV_IN")==0){
        movInInstruccion(contexto_ejecucion, instruccion,direccion,logger_cpu,fd_memoria);
    } else if (strcmp(instruccion.opcode,"MOV_OUT")==0){
        movOutInstruccion(contexto_ejecucion,instruccion,direccion,fd_memoria,logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_OPEN") == 0) {
        flag_ciclo = false;
        fOpenInstruccion(contexto_ejecucion, instruccion, fd_dispatch, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_CLOSE") == 0) {
        flag_ciclo = false;
        fCloseInstruccion(contexto_ejecucion, instruccion, fd_dispatch, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_SEEK") == 0) {
        flag_ciclo = false;
        fSeekInstruccion(contexto_ejecucion, instruccion,direccion, fd_dispatch, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_READ") == 0) {
        flag_ciclo = false;
        fReadInstruccion(contexto_ejecucion, instruccion,direccion, fd_dispatch,fd_memoria, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_WRITE") == 0) {
        flag_ciclo = false;
        fWriteInstruccion(contexto_ejecucion, instruccion,direccion, fd_dispatch,fd_memoria, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_TRUNCATE") == 0) {
        flag_ciclo = false;
        fTruncateInstruccion(contexto_ejecucion, instruccion, fd_dispatch, logger_cpu);
    }
    else if (strcmp(instruccion.opcode, "JNZ") == 0) {
        jnzInstruccion(contexto_ejecucion, instruccion, logger_cpu);
    }
}

DireccionFisica decodeInstruccion(Instruccion *instruccion, pcb* contexto, int fd_dispatch){
    //log_info(logger_cpu,ANSI_COLOR_BLUE "Decoding instruccion");
    if (strcmp(instruccion->opcode, "MOV_IN") == 0 || strcmp(instruccion->opcode, "F_READ") == 0 || 
    strcmp(instruccion->opcode, "F_WRITE") == 0 || strcmp(instruccion->opcode, "MOV_OUT") == 0){
        int direccion_logica = obtener_direccion_logica(instruccion);
        DireccionFisica direccion_fisica = traducir(direccion_logica, conexion_cpu_memoria, contexto->pid, fd_dispatch);
        if(direccion_fisica.marco >= 0){
            return direccion_fisica;
        }else{
            direccion_fisica.marco = -1;
            return direccion_fisica;
        }
    }else{
        DireccionFisica direccion_fisica;
        direccion_fisica.marco = -2;
        return direccion_fisica;
    }
        DireccionFisica direccion_fisica;
        direccion_fisica.marco = -3;
        return direccion_fisica;
    //log_info(logger_cpu,ANSI_COLOR_BLUE "Decodificando instrucción: %s %s %s", instruccion->opcode, instruccion->operando1, instruccion->operando2);
}

int obtener_direccion_logica(Instruccion *instruccion){
    int direccion_logica;
    if (strcmp(instruccion->opcode, "MOV_IN") == 0 || strcmp(instruccion->opcode, "F_READ") == 0 || strcmp(instruccion->opcode, "F_WRITE") == 0){
        direccion_logica = atoi(instruccion->operando2);
    } else if (strcmp(instruccion->opcode, "MOV_OUT") == 0)
    {
        direccion_logica = atoi(instruccion->operando1);
    }
    return direccion_logica;
}

bool fetchInstruccion(int fd, pcb* contexto, Instruccion *instruccion, t_log* logger) {
    //log_info (logger,ANSI_COLOR_BLUE "Fetch de instruccion");
    Instruccion aux;
    send_fetch_instruccion(contexto->path,contexto->pc, fd); // Envía paquete para pedir instrucciones
    op_code cop = recibir_operacion(fd);
    if(cop!=ENVIO_INSTRUCCION){
        log_error(logger,"el cop no corresponde a una instruccion %d",cop);
        return false;
    }
    aux = recv_instruccion(fd);
    instruccion->opcode = malloc(sizeof(char) * strlen(aux.opcode) + 1);
    instruccion->operando1 = malloc(sizeof(char) * strlen(aux.operando1) + 1);
    instruccion->operando2 = malloc(sizeof(char) * strlen(aux.operando2) + 1);
    strcpy(instruccion->opcode, aux.opcode);
    strcpy(instruccion->operando1, aux.operando1);
    strcpy(instruccion->operando2, aux.operando2);
    free(aux.opcode);
    free(aux.operando1);
    free(aux.operando2);
    if(instruccion!=NULL) {
        //log_info(logger,ANSI_COLOR_BLUE "Instrucción recibida: %s %s %s", instruccion->opcode, instruccion->operando1, instruccion->operando2);
        log_info(logger, "PID: %d - FETCH - Program Counter: %d", contexto->pid, contexto->pc);
        return true;
    } else {
        log_error(logger,"Error al recibir la instrucción");
        return false;
    }
    return false;
}