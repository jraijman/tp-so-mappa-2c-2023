#include "main.h"

void levantar_config(char* ruta){
    logger_cpu = iniciar_logger("cpu.log", "CPU:");

    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");
    puerto_dispatch = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config,"PUERTO_ESCUCHA_INTERRUPT");

    log_info(logger_cpu,"ip_memoria: %s y el puerto_memoria: %s",ip_memoria,puerto_memoria);
    log_info(logger_cpu,"puerto_dispatch: %s y puerto_interrupt: %s ",puerto_dispatch,puerto_interrupt);
}

void ejecutarInstruccion(ContextoEjecucion *contexto_ejecucion,char* instr, char* arg1, char* arg2) {
    if (strcmp(instr, "SET") == 0) {
        if (strcmp(arg1, "AX") == 0) {
            contexto_ejecucion->registros.ax= atoi(arg2);
        }
    } else if (strcmp(instr, "SUM") == 0) {
        if (strcmp(arg1, "AX") == 0 && strcmp(arg2, "BX") == 0) {
            contexto_ejecucion->registros.ax = contexto_ejecucion->registros.ax + contexto_ejecucion->registros.bx;
        }
    } else if (strcmp(instr, "SUB") == 0) {
        if (strcmp(arg1, "AX") == 0 && strcmp(arg2, "BX") == 0) {
            contexto_ejecucion->registros.ax = contexto_ejecucion->registros.ax - contexto_ejecucion->registros.bx;
        }
    }   
    
}

int main(int argc, char* argv[]) {
    
    // CONFIG y logger
    levantar_config("cpu.config");


    // inicio servidor de dispatch e interrupt para kernel
    fd_cpu_dispatch = iniciar_servidor(logger_cpu,"CPU DISPATCH",NULL,puerto_dispatch);
    fd_cpu_interrupt = iniciar_servidor(logger_cpu,"CPU INTERRUPT",NULL,puerto_interrupt);

    //genero conexion a memoria
    conexion_cpu_memoria = crear_conexion(logger_cpu,"MEMORIA",ip_memoria,puerto_memoria);


    //espero clientes de dispatch e interrupt
    cliente_dispatch = esperar_cliente(logger_cpu,"CPU DISPATCH",fd_cpu_dispatch);
    cliente_interrupt = esperar_cliente(logger_cpu,"CPU INTERRUPT",fd_cpu_interrupt);

    pcb contexto;
    int bytes_recibidos=0;
    Instruccion instruccion;
    //espero clientes kernel y memoria
    while(server_escuchar_cpu(logger_cpu,"CPU",fd_cpu_dispatch,fd_cpu_interrupt)){
    int bytes = recv(cliente_dispatch, (char*)&contexto + bytes_recibidos, sizeof(pcb) - bytes_recibidos, 0);
    if (bytes <= 0) {
        // Manejo de error o desconexión del cliente
        break;
    }
    bytes_recibidos += bytes;
    }
    if (bytes_recibidos == sizeof(pcb)) {
    //Una vez que tengo el pcb del proceso copiado en contexto, hago mi logica de instrucciones y eso
    //Con el contexto de ejecucion, cargo la peticion a memoria
    send_peticion(conexion_cpu_memoria, &contexto.pid);
    while (1) {
    int bytes_instruccion = recv_instruccion(cliente_dispatch, &instruccion);
    if (bytes_instruccion <= 0) {
        // Se terminaron las instrucciones o se produjo un error en la recepción.
        break;
    }
    if(strcmp(instruccion.opcode,"EXIT"))
    {
        contexto.estado=5;
        //Devuelve el contexto
    }
    else
        ejecutarInstruccion(&contexto, instruccion.opcode, instruccion.operando1, instruccion.operando2);
    }
    }
    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_cpu, config);
    liberar_conexion(conexion_cpu_memoria);
}
