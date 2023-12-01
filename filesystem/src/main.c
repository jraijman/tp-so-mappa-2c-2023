#include "main.h"

void levantar_config(char* ruta){
    logger_filesystem = iniciar_logger("filesystem.log", "FILESYSTEM:");

    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");
    puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
    path_fat= config_get_string_value(config,"PATH_FAT");
    path_bloques= config_get_string_value(config,"PATH_BLOQUES");
    path_fcb= config_get_string_value(config,"PATH_FCB");
    cant_bloques_total= atoi(config_get_string_value(config,"CANT_BLOQUES_TOTAL"));
    cant_bloques_swap= atoi(config_get_string_value(config,"CANT_BLOQUES_SWAP"));
    tam_bloque= atoi(config_get_string_value(config,"TAM_BLOQUE"));
    retardo_acceso_bloque= atoi(config_get_string_value(config,"RETARDO_ACCESO_BLOQUE"));
    retardo_acceso_fat= atoi(config_get_string_value(config,"RETARDO_ACCESO_FAT"));

    log_info(logger_filesystem,"Config cargada");
}
bool iniciar_fat(int tamano_fat, char* path_fat){
    FAT entrada;
    FILE* f=fopen(path_fat, "rb+");
    if(f!=NULL){
        fclose(f);
        return true;
    }else{
        int entradas=tamano_fat/sizeof(FAT);
        entrada.entrada_FAT=0;
        f=fopen(path_fat,"wb");
        if(f!=NULL){
        for(int i=0; i<entradas; i++){
        fwrite(&entrada,sizeof(FAT),1,f);
        }
        }else
        {
            fclose(f);
            return false;
        }
        fclose(f);
        return true;
    }
}
bool iniciar_bloques(int tamano_bloques){
BLOQUE bloque;
FILE* f=fopen(path_bloques, 'rb+');
if(f!=NULL){
    swapLibre=0;
    bloqueLibre=0;
    for(int i=0; i<cant_bloques_swap; i++)
    {
        sleep(retardo_acceso_bloque);
        fread(&bloque,tam_bloque,1,f);
        if((strcmp(bloque.info,'0'))){
            swapLibre++;
        }
    }
    for(int i=0;i<(cant_bloques_total-cant_bloques_swap);i++){
        sleep(retardo_acceso_bloque);
        fread(&bloque,tam_bloque,1,f);
        if(strcmp(bloque.info,'0')){
            bloqueLibre++;
        }
    }
    fclose(f);
    return true;
}else{
    strcpy(bloque.info,'0')//bloque libre
    f=fopen(path_fat,'wb');
    if(f!=NULL){
        for(int i=0; i<cant_bloques_total; i++){
        fwrite(&bloque,tamano_bloques,1,f);
        }
        swapLibre=cant_bloques_swap;
        bloqueLibre=(cant_bloques_total-cant_bloques_swap-1);
        fclose(f);
        return true;
    }else{
        fclose(f);
        return false;
    }
}
}
bool liberar_bloquesSWAP(int[]bloques,int cantidad){
    FILE* f=fopen(path_bloques, 'rb+');
    if(f!=NULL){
        BLOQUE bloque;
        strcpy(bloque.info,'0');
        for(int i=0;i<cantidad;i++){
        fseek(f,tam_bloque*bloques[i],SEEK_SET);
        fwrite(&bloque,tam_bloque,1,f);
        swapLibre++;
        }
    }else{return false;}
}
bool reservar_bloquesSWAP(int cant_bloques, int** bloques_reservados){
    if(swapLibre<cant_bloques){return false};
    FILE*f=fopen(path_bloques, 'rb+');
    BLOQUE bloque;
    if(f!=NULL){
        int j=0;
        while(j<cant_bloques)
        {
            fread(&bloque,tam_bloque,1,f);
            sleep(retardo_acceso_bloque);
            if(strcmp(bloque.info,'0')){
                fseek(f,-tam_bloque,SEEK_CUR);
                strcpy(bloque.info,'\0');
                swapLibre--;
                bloques_reservados[j]=(ftell/tam_bloque);
                sleep(retardo_acceso_bloque);
                fwrite(&bloque,tam_bloque,1,f);
                j++
            }
        }
        return true;
    }else{return false;}
}
int main(int argc, char* argv[]) {
    levantar_config("filesystem.config");
    int fd_filesystem = iniciar_servidor(logger_filesystem,NULL,puerto_escucha,"FILESYSTEM");
    int tamano_fat=(cant_bloques_total-cant_bloques_swap)*sizeof(uint32_t);
    if(!iniciar_fat(tamano_fat,path_fat)){
        log_error(logger_filesystem, "Error al crear el archivo FAT");
    }
    if(!iniciar_bloques(tam_bloque)){
        log_error(logger_filesystem, "Error al iniciar el archivo de bloques");
    }
    //espero clientes kernel y memoria
    while(server_escuchar_filesystem(logger_filesystem,"FILESYSTEM",fd_filesystem));

    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_filesystem, config);
    liberar_conexion(conexion_filesystem_memoria);
}