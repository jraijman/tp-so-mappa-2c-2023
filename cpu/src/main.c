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

void ejecutarInstruccion(struct ContextoEjecucion *contexto_ejecucion,char* instr, char* arg1, char* arg2) {
    if (strcmp(instr, "SET") == 0) {
        if (strcmp(arg1, "AX") == 0) {
            contexto_ejecucion->registros[0] = atoi(arg2);
        }
    } else if (strcmp(instr, "SUM") == 0) {
        if (strcmp(arg1, "AX") == 0 && strcmp(arg2, "BX") == 0) {
            contexto_ejecucion->registros[0] = contexto_ejecucion->registros[0] + contexto_ejecucion->registros[1];
        }
    } else if (strcmp(instr, "SUB") == 0) {
        if (strcmp(arg1, "AX") == 0 && strcmp(arg2, "BX") == 0) {
            contexto_ejecucion->registros[0] = contexto_ejecucion->registros[0] - contexto_ejecucion->registros[1];
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

    struct ContextoEjecucion contexto_proceso;
     //espero clientes kernel y memoria
    while(server_escuchar_cpu(logger_cpu,"CPU",fd_cpu_dispatch,fd_cpu_interrupt)){
    int bytes_recibidos = recv(cliente_dispatch, &contexto_proceso, sizeof(struct ContextoEjecucion), 0);
    if (bytes_recibidos == sizeof(struct ContextoEjecucion)) {
    }
    int i=0;
    while(contexto_proceso.instrucciones[i][0]!='/0')
    {
        if(strcmp(contexto_proceso.instrucciones[i],'EXIT') != 0)
        char instruccion[3]=string_split(contexto_proceso.instrucciones[i], " ");
        ejecutarInstruccion(&contexto_proceso,instruccion[1],instruccion[2],instruccion[3]);
        }
        else
        {
            //ejecutar exit
        }
        i++;
    }
    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_cpu, config);
    liberar_conexion(conexion_cpu_memoria);
}
