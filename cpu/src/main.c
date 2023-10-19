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
    } else if (strcmp(instruccion.opcode, "SLEEP") == 0) {
        // Instrucción SLEEP: Bloquear el proceso y devolver el tiempo de bloqueo
        int tiempo_bloqueo;
        strcpy(tiempo_bloqueo,instruccion.operando1);
        enviarPCBa(contexto_ejecucion, fd_cpu_dispatch, "SLEEP", tiempo_bloqueo);
    } else if (strcmp(instruccion.opcode, "WAIT") == 0) {
        char recurso;
        strcpy(recurso, instruccion.operando1);
        enviarPCBa(contexto_ejecucion, fd_cpu_dispatch, "WAIT",recurso);
    } else if (strcmp(instruccion.opcode, "SIGNAL") == 0) {
        // Instrucción SIGNAL: Solicitar liberación de un recurso al Kernel
        char recurso;
        strcpy(recurso, instruccion.operando1);
        enviarPCBa(contexto_ejecucion, fd_cpu_dispatch, "SIGNAL",recurso);
    }
    else if (strcmp(instruccion.opcode, "EXIT") == 0) {
        //sacar despues de definir algo porque esta puesto para que no tire error
        int algo;
        contexto_ejecucion->estado=5;
        enviarPCBa(contexto_ejecucion, fd_cpu_dispatch, "EXIT", algo);
    }
}
char* traducir(char* direccion_logica){
    // Calcula el número de página y el desplazamiento
    int numero_pagina = atoi(direccion_logica) / TAMANO_PAGINA;//no esta definido tamanio de pagina
    int desplazamiento = direccion_logica - numero_pagina * TAMANO_PAGINA;

    int numero_marco = pedir_marco(conexion_cpu_memoria, numero_pagina);
    
    int direccion_fisica = numero_marco * TAMANO_PAGINA + desplazamiento;

    return direccion_fisica;
}

void decodeInstruccion(Instruccion instruccion) {
    if (strcmp(instruccion.opcode, "MOV_IN") == 0 ||
        strcmp(instruccion.opcode, "F_READ") == 0 || strcmp(instruccion.opcode, "F_WRITE") == 0){
        strcpy(instruccion.operando2,traducir(instruccion.operando2));
    }else if (strcmp(instruccion.opcode, "MOV_OUT") == 0)
    {
        strcpy(instruccion.operando1,traducir(instruccion.operando1));
    }
    return;
}

bool fetchInstruccion(int fd, pcb contexto, Instruccion* instruccion, t_log* logger) {
    if (recv_instruccion(fd, instruccion, logger)) {
        log_info(logger, "PID: %d - FETCH - Program Counter: %d", contexto.pid, contexto.pc);
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
    int interrupciones = 0;
    bool instrucciones=true;
    // Espero al kernel
    while (server_escuchar_cpu(logger_cpu, "CPU", fd_cpu_dispatch, fd_cpu_interrupt)) {
        int bytes = recv(cliente_dispatch, (char*)&contexto + bytes_recibidos, sizeof(pcb) - bytes_recibidos, 0);
        if (bytes <= 0) {
            // Manejo de error o desconexión del cliente
            break;
        }
        bytes_recibidos += bytes;
    }
    if (bytes_recibidos == sizeof(pcb)) {
        // Ya tengo el PCB, entonces voy a pedir instrucciones hasta que llegue una interrupción que desaloje al proceso.
        while (interrupciones <= 0 && instrucciones) {
            if (fetchInstruccion(conexion_cpu_memoria, contexto, &instruccion, logger_cpu)) {
                decodeInstruccion(instruccion, &pageFault);
                if(strcmp(instruccion.operando1,"PAGE FAULT")==0 || strcmp(instruccion.operando2,"PAGE FAULT")==0)
                {
                    //Manejo de page fault
                }
                else{
                    contexto.pc++;
                    executeInstruccion(&contexto, instruccion);
                }
        //Veo si llegó alguna interrupcion mientras ejecutaba
            if (cliente_interrupt != -1) 
                interrupciones++;  
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
}   
