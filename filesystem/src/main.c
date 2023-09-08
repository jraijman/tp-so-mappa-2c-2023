#include <main.h>

void levantar_config(char* ruta){
    logger_filesystem = iniciar_logger("filesystem.log", "FILESYSTEM:");

    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");
    puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
    path_fat= config_get_string_value(config,"PATH_FAT");
    path_bloques= config_get_string_value(config,"PATH_BLOQUES");
    path_fcb= config_get_string_value(config,"PATH_FCB");
    cant_bloques_total= config_get_string_value(config,"CANT_BLOQUES_TOTAL");
    cant_bloques_swap= config_get_string_value(config,"CANT_BLOQUES_SWAP");
    tam_bloque= config_get_string_value(config,"TAM_BLOQUE");
    retardo_acceso_bloque= config_get_string_value(config,"RETARDO_ACCESO_BLOQUE");
    retardo_acceso_fat= config_get_string_value(config,"RETARDO_ACCESO_FAT");

    log_info(logger_filesystem,"Config cargada");
}


int main(int argc, char* argv[]) {


    // CONFIG y logger
    levantar_config("filesystem.config");
    
    // inicio servidor de escucha
    fd_filesystem = iniciar_servidor(logger_filesystem,"FILESYSTEM",NULL,puerto_escucha);

    //genero conexion a memoria
    conexion_filesystem_memoria = crear_conexion(logger_filesystem,"MEMORIA",ip_memoria,puerto_memoria);

    //espero clientes kernel y memoria
    while(server_escuchar_filesystem(logger_filesystem,"FILESYSTEM",fd_filesystem));

    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_filesystem, config);
    liberar_conexion(conexion_filesystem_memoria);
}
