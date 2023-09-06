#include <main.h>

void levantar_config(char* ruta){
    logger_memoria = iniciar_logger("memoria.log", "MEMORIA:");

    config = iniciar_config(ruta);
    puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
    ip_filesystem = config_get_string_value(config,"IP_FILESYSTEM");
    tam_memoria = config_get_string_value(config,"TAM_MEMORIA");
    puerto_filesystem= config_get_string_value(config,"PUERTO_FILESYSTEM");
    tam_pagina= config_get_string_value(config,"TAM_PAGINA");
    path_instrucciones = config_get_string_value(config,"PATH_INSTRUCCIONES");
    retardo_respuesta = config_get_string_value(config,"RETARDO_RESPUESTA");
    algoritmo_reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
    
    log_info(logger_memoria,"Config cargada");
}

int main(int argc, char* argv[]) {
    

    // CONFIG y logger
    levantar_config("memoria.config");
    // inicio servidor de escucha
    fd_memoria= iniciar_servidor(logger_memoria,"MEMORIA",NULL,puerto_escucha);

    //genero conexion a filesystem
    conexion_memoria_filesystem = crear_conexion(logger_memoria,"FILESYSTEM",ip_filesystem,puerto_filesystem);

    //espero clientes kernel,cpu y filesystem
    while(server_escuchar_memoria(logger_memoria,"MEMORIA",fd_memoria));

    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_memoria, config);
    liberar_conexion(conexion_memoria_filesystem);
}
