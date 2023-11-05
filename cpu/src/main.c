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

void levantar_config(char* ruta){
    logger_cpu = iniciar_logger("cpu.log", "CPU:");
    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    log_info(logger_cpu, "Módulo CPU iniciado correctamente");
    log_info(logger_cpu, "ip_memoria: %s y el puerto_memoria: %s", ip_memoria, puerto_memoria);
    log_info(logger_cpu, "puerto_dispatch: %s y puerto_interrupt: %s", puerto_dispatch, puerto_interrupt);
}

void ciclo_instruccion(pcb* contexto, int cliente_socket_dispatch, int cliente_socket_interrupt, t_log* logger) {
    log_info(logger, "Inicio del ciclo de instrucción");
    while (cliente_socket_dispatch != -1) {
        while (cliente_socket_interrupt != -1) {
            Instruccion instruccion;
            fetchInstruccion(conexion_cpu_memoria, contexto, &instruccion, logger);
            contexto->pc++;
            log_info(logger, "Instrucción recibida");
            decodeInstruccion(&instruccion);
            executeInstruccion(contexto, instruccion);
        }
        pcbDesalojado interrumpido;
        interrumpido.contexto = contexto;
        strcpy(interrumpido.extra, "INTERRUPCIÓN");
        //send_pcbDesalojado(interrumpido, cliente_socket_dispatch);
        // NO SÉ SI AL KERNEL DEBERÍA ENVIARLO POR EL DISPATCH O SI PASA A TRAVÉS DE MEMORIA
    }
}

void executeInstruccion(pcb* contexto_ejecucion, Instruccion instruccion) {
    log_info(logger_cpu, "Instrucción ejecutada: %s %s %s", instruccion.opcode, instruccion.operando1, instruccion.operando2);

    pcbDesalojado pcbDes;
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
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "SLEEP";
        pcbDes.extra = instruccion.operando1;
        //send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    } else if (strcmp(instruccion.opcode, "WAIT") == 0) {
        log_info(logger_cpu, "Instrucción WAIT - Proceso está esperando por recurso: %s", instruccion.operando1);
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "WAIT";
        pcbDes.extra = instruccion.operando1;
        //send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    } else if (strcmp(instruccion.opcode, "SIGNAL") == 0) {
        log_info(logger_cpu, "Instrucción SIGNAL - Proceso ha liberado recurso: %s", instruccion.operando1);
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "SIGNAL";
        pcbDes.extra = instruccion.operando1;
        //send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    } else if (strcmp(instruccion.opcode, "EXIT") == 0) {
        log_info(logger_cpu, "Instrucción EXIT - Proceso ha finalizado su ejecución");
        pcbDes.contexto = contexto_ejecucion;
        pcbDes.instruccion = "EXIT";
        pcbDes.extra = "";
        //send_pcbDesalojado(pcbDes, fd_cpu_dispatch);
    }
}

void traducir(Instruccion *instruccion, Direccion *direccion) {
/*
    if(strcmp(instruccion->opcode,"MOV_OUT"))
    {
        direccion->direccionLogica=atoi(instruccion->operando1);
            send_direccion(conexion_cpu_memoria, direccion);
            recv_direccion(conexion_cpu_memoria, direccion);
    }else{
        direccion->direccionLogica=atoi(instruccion->operando2);
            send_direccion(conexion_cpu_memoria, direccion);
            recv_direccion(conexion_cpu_memoria, direccion);
    }
    //TODO LOGICA DEL CALCULO HAY QUE VER EN MEMORIA COMO MANEJAMOS LA ESTRUCTURA
*/
}


void decodeInstruccion(Instruccion *instruccion){
    log_info(logger_cpu, "Decoding instruccion");
    Direccion direccion;
    if (strcmp(instruccion->opcode, "MOV_IN") == 0 || strcmp(instruccion->opcode, "F_READ") == 0 || 
    strcmp(instruccion->opcode, "F_WRITE") == 0 || strcmp(instruccion->opcode, "MOV_OUT") == 0){
        traducir(instruccion, &direccion);
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