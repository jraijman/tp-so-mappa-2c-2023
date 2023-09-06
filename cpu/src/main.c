#include <main.h>

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


int main(int argc, char* argv[]) {
    
    // CONFIG y logger
    levantar_config("cpu.config")


    // inicio servidor de dispatch e interrupt para kernel
    fd_cpu_dispatch = iniciar_servidor(logger_cpu,"CPU DISPATCH",NULL,puerto_dispatch);
    fd_cpu_interrupt = iniciar_servidor(logger_cpu,"CPU INTERRUPT",NULL,puerto_interrupt);

    //espero clientes de dispatch e interrupt
    cliente_dispatch = esperar_cliente(logger_cpu,"CPU DISPATCH",fd_cpu_dispatch);
    cliente_interrupt = esperar_cliente(logger_cpu,"CPU INTERRUPT",fd_cpu_interrupt);


    //genero conexion a memoria
    conexion_cpu_memoria = crear_conexion(logger_cpu,"MEMORIA",ip_memoria,puerto_memoria);


    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_cpu, config);
    liberar_conexion(conexion_cpu_memoria);
}
