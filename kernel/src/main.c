#include <main.h>

int main(int argc, char* argv[]) {
    
    int conexion_dispatch;
    int conexion_interrupt;
	char* ip_memoria;
    char* ip_cpu;
	char* puerto_memoria;
	char* ip_filesystem;
	char* puerto_filesystem;
    char* puerto_cpu_interrupt;
    char* puerto_cpu_dispatch;

    t_log* logger_kernel;
	t_config* config;

    logger_kernel = iniciar_logger("kernel.log", "KERNEL:");

    // CONFIG
    config = iniciar_config("kernel.config");
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    ip_cpu = config_get_string_value(config,"IP_CPU");
    ip_filesystem = config_get_string_value(config,"IP_FILESYSTEM");
    puerto_filesystem= config_get_string_value(config,"PUERTO_FILESYSTEM");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");
    puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
    log_info(logger_kernel,"ip_memoria: %s y el puerto_memoria: %s",ip_memoria,puerto_memoria);
    log_info(logger_kernel,"puerto_dispatch: %s y puerto_interrupt: %s ",puerto_cpu_dispatch,puerto_cpu_interrupt);

    conexion_dispatch = crear_conexion(logger_kernel,"CPU_DISPATCH",ip_cpu,puerto_cpu_dispatch);
    conexion_interrupt = crear_conexion(logger_kernel,"CPU_INTERRUPT",ip_cpu,puerto_cpu_interrupt);

    terminar_programa(logger_kernel, config);
    liberar_conexion(conexion_dispatch);
    liberar_conexion(conexion_interrupt);
  
}
