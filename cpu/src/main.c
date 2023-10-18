#include "main.h"

void levantar_config(char* ruta) {
    logger_cpu = iniciar_logger("cpu.log", "CPU:");

    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

    log_info(logger_cpu, "ip_memoria: %s y el puerto_memoria: %s", ip_memoria, puerto_memoria);
    log_info(logger_cpu, "puerto_dispatch: %s y puerto_interrupt: %s", puerto_dispatch, puerto_interrupt);
}

void executeInstruccion(pcb* contexto_ejecucion, Instruccion instruccion) {
    log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", contexto_ejecucion->pid, instruccion.opcode, instruccion.operando1, instruccion.operando2);

    if (strcmp(instruccion.opcode, "SET") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0) {
            contexto_ejecucion->registros.ax = atoi(instruccion.operando2);
        }
    } else if (strcmp(instruccion.opcode, "SUM") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0 && strcmp(instruccion.operando2, "BX") == 0) {
            contexto_ejecucion->registros.ax = contexto_ejecucion->registros.ax + contexto_ejecucion->registros.bx;
        }
    } else if (strcmp(instruccion.opcode, "SUB") == 0) {
        if (strcmp(instruccion.operando1, "AX") == 0 && strcmp(instruccion.operando2, "BX") == 0) {
            contexto_ejecucion->registros.ax = contexto_ejecucion->registros.ax - contexto_ejecucion->registros.bx;
        }
    }
}

void decodeInstruccion(Instruccion instruccion, bool &pageFault) {
    if (strcmp(instruccion.opcode, "MOV_IN") == 0 || strcmp(instruccion.opcode, "MOV_OUT") == 0 ||
        strcmp(instruccion.opcode, "F_READ") == 0 || strcmp(instruccion.opcode, "F_WRITE") == 0 ||
        strcmp(instruccion.opcode, "F_TRUNCATE") == 0) {
        pageFault=traducir(&instruccion);
    }
}

bool fetchInstruccion(int fd, pcb contexto, Instruccion* instruccion, t_log* logger) {
    if (recv_instruccion(fd, instruccion, logger)) {
        log_info(logger, "PID: %d - FETCH - Program Counter: %d", contexto.pid, contexto.pc);
        contexto.pc++;
        return true;
    } else {
        log_error(logger, "Error al recibir la instrucción");
        return false;
    }
}

int main(int argc, char* argv[]) {
    // CONFIG y logger
    levantar_config("cpu.config");
    logger_cpu = iniciar_logger("cpu.log", "CPU:");

    // Inicio servidor de dispatch e interrupt para el kernel
    fd_cpu_dispatch = iniciar_servidor(logger_cpu, "CPU DISPATCH", NULL, puerto_dispatch);
    fd_cpu_interrupt = iniciar_servidor(logger_cpu, "CPU INTERRUPT", NULL, puerto_interrupt);

    // Genero conexión a memoria
    conexion_cpu_memoria = crear_conexion(logger_cpu, "MEMORIA", ip_memoria, puerto_memoria);

    // Espero clientes de dispatch e interrupt
    cliente_dispatch = esperar_cliente(logger_cpu, "CPU DISPATCH", fd_cpu_dispatch);
    cliente_interrupt = esperar_cliente(logger_cpu, "CPU INTERRUPT", fd_cpu_interrupt);

    pcb contexto;
    int bytes_recibidos = 0;
    Instruccion instruccion;

    // Espero al kernel
    while (server_escuchar_cpu(logger_cpu, "CPU", fd_cpu_dispatch, fd_cpu_interrupt)) {
        int interrupciones = 0;
        int bytes = recv(cliente_dispatch, (char*)&contexto + bytes_recibidos, sizeof(pcb) - bytes_recibidos, 0);
        if (bytes <= 0) {
            // Manejo de error o desconexión del cliente
            break;
        }
        bytes_recibidos += bytes;
    }
    bool instrucciones=true;
    bool pageFault=false;
    if (bytes_recibidos == sizeof(pcb)) {
        // Ya tengo el PCB, entonces voy a pedir instrucciones hasta que llegue una interrupción que desaloje al proceso.
        while (interrupciones <= 0 && instrucciones) {
            if (fetchInstruccion(conexion_cpu_memoria, contexto, &instruccion, logger_cpu)) {
                decodeInstruccion(instruccion, &pageFault)
                if(pageFault==false)
                {
                    executeInstruccion(&contexto, instruccion);
                }
                else{
                    //manejo de page fault
                }
            } else {
                instrucciones=false;
                break;
            }
        //Veo si llegó alguna interrupcion mientras ejecutaba
            if (cliente_socket_interrupt != -1) {
                interrupciones++;}  
        }
        if (interrupciones != 0)
        {
            //lógica para devolver el contexto e iniciar la rutina de interrupcion
        }
        else
        {
            //Se acabaron las instrucciones a ejecutar por lo que finaliza el programa
        }    
        // CIERRO LOG Y CONFIG y libero conexión
        terminar_programa(logger_cpu, config);
        liberar_conexion(conexion_cpu_memoria);
    }
}
