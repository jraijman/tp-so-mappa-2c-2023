#include <main.h>

int main(int argc, char* argv[]) {
    
    int conexion;
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_dispatch;
    char* puerto_interrupt;

    t_log* logger_cpu;
	t_config* config;

    logger_cpu = iniciar_logger("cpu.log", "CPU:");

    // CONFIG
    config = iniciar_config("cpu.config");
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");
    puerto_dispatch = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config,"PUERTO_ESCUCHA_INTERRUPT");
    log_info(logger_cpu,"ip_memoria: %s y el puerto_memoria: %s",ip_memoria,puerto_memoria);
    log_info(logger_cpu,"puerto_dispatch: %s y puerto_interrupt: %s ",puerto_dispatch,puerto_interrupt);

    int fd_cpu_dispatch = iniciar_servidor(logger_cpu,"CPU DISPATCH",NULL,puerto_dispatch);
    int fd_cpu_interrupt = iniciar_servidor(logger_cpu,"CPU INTERRUPT",NULL,puerto_interrupt);

    int cliente_dispatch = esperar_cliente(logger_cpu,"CPU DISPATCH",fd_cpu_dispatch);
    int cliente_interrupt = esperar_cliente(logger_cpu,"CPU DISPATCH",fd_cpu_interrupt);


    terminar_programa(logger_cpu, config);
  
}
