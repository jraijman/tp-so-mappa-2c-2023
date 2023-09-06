#include <main.h>

void levantar_config(char* ruta){
    logger_kernel = iniciar_logger("kernel.log", "KERNEL:");

    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    ip_cpu = config_get_string_value(config,"IP_CPU");
    ip_filesystem = config_get_string_value(config,"IP_FILESYSTEM");
    puerto_filesystem= config_get_string_value(config,"PUERTO_FILESYSTEM");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");
    puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
    algoritmo_planificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
    quantum = config_get_string_value(config,"QUANTUM");
    //es una lista VER
    recursos = config_get_string_value(config,"RECURSOS");
    //es una lista VER
    instancia_recursos = config_get_string_value(config,"INSTANCIAS_RECURSOS");
    grado_multiprogramacion = config_get_string_value(config,"GRADO_MULTIPROGRAMACION_INI");
    
    log_info(logger_kernel,"Config cargada");
}

int main(int argc, char* argv[]) {
    
    

    // CONFIG y logger
    levantar_config("kernel.config");
/*
    // conexiones a cpu
    conexion_dispatch = crear_conexion(logger_kernel,"CPU_DISPATCH",ip_cpu,puerto_cpu_dispatch);
    conexion_interrupt = crear_conexion(logger_kernel,"CPU_INTERRUPT",ip_cpu,puerto_cpu_interrupt);

    //conexion a memoria
    conexion_memoria = crear_conexion(logger_kernel,"MEMORIA",ip_memoria,puerto_memoria);
*/
    //conexion a FileSystem
    conexion_fileSystem = crear_conexion(logger_kernel,"FILESYSTEM",ip_filesystem,puerto_filesystem);

    //mando mensaje de prueba
    send_aprobar_operativos(conexion_fileSystem, 1, 14);


    // libero conexiones, log y config
    terminar_programa(logger_kernel, config);
    liberar_conexion(conexion_dispatch);
    liberar_conexion(conexion_interrupt);
    liberar_conexion(conexion_memoria);
    liberar_conexion(conexion_fileSystem);
}
