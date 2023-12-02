#include "main.h"

int main(int argc, char* argv[]){
    // CONFIG y logger
    levantar_config("cpu.config");
    logger_cpu = iniciar_logger("cpu.log", "CPU:");


    // Genero conexión a memoria
    conexion_cpu_memoria = crear_conexion(logger_cpu, "MEMORIA", ip_memoria, puerto_memoria);  
    //mensaje prueba 
    enviar_mensaje("Hola, soy CPU!", conexion_cpu_memoria);


    // Inicio servidor de dispatch e interrupt para el kernel
    fd_cpu_dispatch = iniciar_servidor(logger_cpu, NULL, puerto_dispatch, "CPU DISPATCH");
    fd_cpu_interrupt = iniciar_servidor(logger_cpu, NULL, puerto_interrupt, "CPU INTERRUPT");
    
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
                log_info(logger_cpu, ANSI_COLOR_YELLOW "Recibí una interrupcion");
                int pid_recibido;
                recv_interrupcion(cliente_socket_interrupt,pid_recibido );
                recibio_interrupcion = true;
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
	while (cliente_socket_dispatch != -1) {
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
                log_info(logger_cpu, ANSI_COLOR_YELLOW "Recibí un paquete con los siguientes valores: ");
                break;
            case ENVIO_PCB: {
                pcb* contexto=recv_pcb(cliente_socket_dispatch);
                if (contexto->pid!=-1) {
                    log_info(logger_cpu, ANSI_COLOR_YELLOW "Recibí PCB con ID: %d", contexto->pid);
                    enviar_mensaje("deberia mandar pcb desalojado", cliente_socket_dispatch);
                    sleep(0.99);
                    ciclo_instruccion(contexto, cliente_socket_dispatch,cliente_socket_dispatch, logger_cpu);
                    
                } else {
                    log_error(logger_cpu, "Error al recibir el PCB");
                    
                }
                break;
            }
            default: {
                log_error(logger_cpu, "Código de operación no reconocido en Dispatch: %d", cop);
                break;
            }
        }
    }
}

int server_escuchar(int fd_cpu_interrupt, int fd_cpu_dispatch) {
	int socket_cliente_interrupt = esperar_cliente(logger_cpu, "CPU INTERRUPT", fd_cpu_interrupt);
    char * server_name = "Cpu Dispatch";
    int socket_cliente_dispatch = esperar_cliente(logger_cpu, server_name, fd_cpu_dispatch);
	
    if (socket_cliente_interrupt != -1 && socket_cliente_dispatch != -1) {
        int *args = malloc(sizeof(int));
        //hilo para servidor dispatch
        pthread_t hilo_dispatch;
		args = &socket_cliente_dispatch;
		pthread_create(&hilo_dispatch, NULL, (void*) procesar_conexion_dispatch, (void*) args);
		pthread_detach(hilo_dispatch);

        //hilo para servidor interrupt
        pthread_t hilo_interrupt;
		args = &socket_cliente_interrupt;
		pthread_create(&hilo_interrupt, NULL, (void*) procesar_conexion_interrupt, (void*) args);
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
    log_info(logger,ANSI_COLOR_BLUE "Inicio del ciclo de instrucción");
    while (cliente_socket_dispatch != -1) {
            Instruccion * instruccion = malloc(sizeof(Instruccion));
            fetchInstruccion(conexion_cpu_memoria, contexto, instruccion, logger);
            contexto->pc++;
            decodeInstruccion(instruccion,contexto);
            executeInstruccion(contexto, *instruccion, cliente_socket_dispatch, conexion_cpu_memoria);
            if (strcmp(instruccion->opcode, "EXIT") == 0) {
                return;
            }
            if(recibio_interrupcion){
                recibio_interrupcion=false;
                send_pcbDesalojado(contexto,"INTERRUPCION","",cliente_socket_dispatch, logger);
                return;
            }
        return;
    }
}

void executeInstruccion(pcb* contexto_ejecucion, Instruccion instruccion, int fd_dispatch, int fd_memoria) {
    log_info(logger_cpu, "Instrucción ejecutada: %s %s %s", instruccion.opcode, instruccion.operando1, instruccion.operando2);

    if (strcmp(instruccion.opcode, "SET") == 0) {
        setInstruccion(contexto_ejecucion, instruccion, logger_cpu);
    } else if (strcmp(instruccion.opcode, "SUM") == 0) {
        sumInstruccion(contexto_ejecucion, instruccion, logger_cpu);
    } else if (strcmp(instruccion.opcode, "SUB") == 0) {
        subInstruccion(contexto_ejecucion, instruccion, logger_cpu);
    } else if (strcmp(instruccion.opcode, "SLEEP") == 0) {
        sleepInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode, "WAIT") == 0) {
        waitInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode, "SIGNAL") == 0) {
        signalInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode, "EXIT") == 0) {
        exitInstruccion(contexto_ejecucion, instruccion, logger_cpu,fd_dispatch);
    } else if (strcmp(instruccion.opcode,"MOV_IN")==0){
        movInInstruccion(contexto_ejecucion, instruccion,logger_cpu);
    } else if (strcmp(instruccion.opcode,"MOV_OUT")==0){
        movOutInstruccion(contexto_ejecucion,instruccion,fd_memoria,logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_OPEN") == 0) {
        fOpenInstruccion(contexto_ejecucion, instruccion, fd_cpu_dispatch, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_CLOSE") == 0) {
        fCloseInstruccion(contexto_ejecucion, instruccion, fd_cpu_dispatch, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_SEEK") == 0) {
        fSeekInstruccion(contexto_ejecucion, instruccion, fd_cpu_dispatch, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_READ") == 0) {
        fReadInstruccion(contexto_ejecucion, instruccion, fd_cpu_dispatch,fd_memoria, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_WRITE") == 0) {
        fWriteInstruccion(contexto_ejecucion, instruccion, fd_cpu_dispatch,fd_memoria, logger_cpu);
    } else if (strcmp(instruccion.opcode, "F_TRUNCATE") == 0) {
        fTruncateInstruccion(contexto_ejecucion, instruccion, fd_cpu_dispatch, logger_cpu);
    }
}

void decodeInstruccion(Instruccion *instruccion, pcb* contexto){
    log_info(logger_cpu,ANSI_COLOR_BLUE "Decoding instruccion");
    Direccion direccion;
    if (strcmp(instruccion->opcode, "MOV_IN") == 0 || strcmp(instruccion->opcode, "F_READ") == 0 || 
    strcmp(instruccion->opcode, "F_WRITE") == 0 || strcmp(instruccion->opcode, "MOV_OUT") == 0){
        //traducir(instruccion, &direccion, contexto);
    }
    log_info(logger_cpu,ANSI_COLOR_BLUE "Decodificando instrucción: %s %s %s", instruccion->opcode, instruccion->operando1, instruccion->operando2);
}

bool fetchInstruccion(int fd, pcb* contexto, Instruccion *instruccion, t_log* logger) {
    log_info (logger,ANSI_COLOR_BLUE "Fetch de instruccion");
    Instruccion aux;
    send_fetch_instruccion(contexto->path,contexto->pc, fd); // Envía paquete para pedir instrucciones
    recibir_operacion(fd);
    aux=recv_instruccion(fd);
    instruccion->opcode = malloc(sizeof(char) * strlen(aux.opcode) + 1);
    instruccion->operando1 = malloc(sizeof(char) * strlen(aux.operando1) + 1);
    instruccion->operando2 = malloc(sizeof(char) * strlen(aux.operando2) + 1);
    strcpy(instruccion->opcode, aux.opcode);
    strcpy(instruccion->operando1, aux.operando1);
    strcpy(instruccion->operando2, aux.operando2);
    if (instruccion!=NULL) {
        log_info(logger,ANSI_COLOR_BLUE "Instrucción recibida: %s %s %s", instruccion->opcode, instruccion->operando1, instruccion->operando2);
        log_info(logger, "PID: %d - FETCH - Program Counter: %d", contexto->pid, contexto->pc);
        return true;
    } else {
        perror("Error al recibir la instrucción");
        return false;
    }
}