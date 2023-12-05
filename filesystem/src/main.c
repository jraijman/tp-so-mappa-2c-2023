#include "main.h"

//----------------------------CONFIG----------------------------------------
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
//----------------------------GESTION SWAP----------------------------------
bool liberar_bloquesSWAP(int bloques[],int cantidad, bool* bitmap){
    FILE* f=fopen(path_bloques, "rb+");
    if(f!=NULL){
        BLOQUE bloque;
        bloque.info=malloc(tam_bloque);
        strcpy(bloque.info,"0");
        for(int i=0;i<cantidad;i++){
            fseek(f,tam_bloque*bloques[i],SEEK_SET);
            sleep(retardo_acceso_bloque/1000);
            log_info(logger_filesystem, "ACCESO A BLOQUE SWAP NRO: %ld",ftell(f)/tam_bloque);
            bitmap[bloques[i]]=0;
            fwrite(bloque.info,tam_bloque,1,f);
            swapLibres++;
        }
        fclose(f);
        free(bloque.info);
        return true;
    }else{fclose(f);return false;}
}

bool reservar_bloquesSWAP(int cant_bloques, int bloques_reservados[], bool* bitmap) {
    if (swapLibres < cant_bloques) {
        return false;
    }

    FILE* f = fopen(path_bloques, "rb+");
    fseek(f, 0, SEEK_SET);
    BLOQUE bloque;
    bloque.info = malloc(tam_bloque);

    if (f != NULL) {
        int j = 0;
        while (j < cant_bloques) {
            sleep(retardo_acceso_bloque / 1000);
            fread(bloque.info, tam_bloque, 1, f);
            log_info(logger_filesystem, "ACCESO A BLOQUE SWAP NRO: %ld", (ftell(f) / tam_bloque));

            if (strcmp(bloque.info, "0") == 0) {
                strcpy(bloque.info, "\0");
                swapLibres--;
                bloques_reservados[j] = (ftell(f) / tam_bloque);
                sleep(retardo_acceso_bloque / 1000);
                bitmap[ftell(f) / tam_bloque] = 1;
                fwrite(bloque.info, tam_bloque, 1, f);
                fseek(f, tam_bloque, SEEK_CUR);
                log_info(logger_filesystem, "ACCESO A BLOQUE SWAP NRO: %ld", (ftell(f) / tam_bloque));
                j++;
            } else {
                fseek(f, tam_bloque, SEEK_CUR);
            }
        }

        fclose(f);
        return true;
    } else {
        return false;
    }
}

//----------------------------GESTION BLOQUES----------------------------------
bool iniciar_bloques(int tamano_bloques, bool* bitmapBloques, bool* bitmapSwap) {
    BLOQUE bloque;
    bloque.info = malloc(tamano_bloques);
    strcpy(bloque.info,"0");  // Inicializa el bloque con el carácter '0'

    FILE* f = fopen(path_bloques, "rb+");

    if (f != NULL) {
        swapLibres = 0;
        bloquesLibres = 0;

        for (int i = 0; i < cant_bloques_swap; i++) {
            fread(bloque.info, 1, tamano_bloques, f);

            if (strcmp(bloque.info, "0")==0) {
                swapLibres++;
            } else {
                bitmapSwap[i] = 1;
            }
        }

        for (int i = 0; i < (cant_bloques_total - cant_bloques_swap); i++) {
            fread(bloque.info, 1, tamano_bloques, f);

            if (strcmp(bloque.info, "0") == 0) {
                bloquesLibres++;
            } else {
                bitmapBloques[i] = 1;
            }
        }

        bloquesLibres--;

        free(bloque.info);
        fclose(f);
        return true;
    } else {
        f = fopen(path_bloques, "wb");
        
        if (f != NULL) {
            for (int i = 0; i < cant_bloques_total; i++) {
                fwrite(bloque.info, tamano_bloques, 1, f);
            }

            swapLibres = cant_bloques_swap;
            bloquesLibres = (cant_bloques_total - cant_bloques_swap - 1);

            free(bloque.info);
            fclose(f);
            return true;
        } else {
            free(bloque.info);
            fclose(f);
            return false;
        }
    }
}

void bloquesOcupados(bool* bitmap){
    for(int i=0;i<cant_bloques_total-cant_bloques_swap;i++)
    {
        if(bitmap[i]==1)
        {
            log_info(logger_filesystem,"EL BLOQUE %d ESTA OCUPADO",i);
        }
    }
}
void reservarBloque(FILE* f, uint32_t bloque, bool* bitmap) {
    fseek(f,(bloque+cant_bloques_swap)*tam_bloque, SEEK_SET);
    BLOQUE reservado;
    reservado.info = malloc(tam_bloque);
    strcpy(reservado.info, "\0");
    sleep(retardo_acceso_bloque / 1000);
    fwrite(reservado.info, tam_bloque, 1, f);
    log_info(logger_filesystem, "ACCESO A BLOQUE NRO: %ld", ftell(f)/tam_bloque-cant_bloques_swap);
    bitmap[bloque] = 1;
    bloquesLibres--;    
    free(reservado.info);
}


void liberarBloque(FILE* f, uint32_t bloqueLib, bool* bitmap) {
    fseek(f, (bloqueLib+cant_bloques_swap)*tam_bloque, SEEK_SET);
    
    BLOQUE bloque;
    bloque.info = malloc(tam_bloque);
    strcpy(bloque.info, "0");
    sleep(retardo_acceso_bloque / 1000);
    fwrite(bloque.info, tam_bloque, 1, f);
    log_info(logger_filesystem, "ACCESO A BLOQUE NRO: %ld", ftell(f)/tam_bloque-cant_bloques_swap);
    bitmap[bloqueLib] = 0;
    bloquesLibres++
    free(bloque.info);
}

//----------------------------GESTION FCBs-------------------------------------
bool crear_archivo(char* nombre) {
    char nombreArchivo[strlen(nombre) + 1];
    strcpy(nombreArchivo, nombre);
    int tam = strlen(path_fcb) + strlen(nombre) + 4;
    char ruta[tam];
    strcpy(ruta, path_fcb);
    strcat(ruta, nombre);
    strcat(ruta, ".fcb");
    t_config* nuevoFCB=config_create(ruta);
    if(nuevoFCB!=NULL){
        return false;
    }else{
        nuevoFCB=(t_config*)malloc(sizeof(t_config));
        nuevoFCB->properties=dictionary_create();
        nuevoFCB->path=ruta;
        config_save_in_file(nuevoFCB,ruta);
        nuevoFCB=config_create(ruta);
        config_set_value(nuevoFCB,"NOMBRE_ARCHIVO", strdup(nombreArchivo));
        config_set_value(nuevoFCB,"TAMANIO_ARCHIVO", "0");
        config_set_value(nuevoFCB, "BLOQUE_INICIAL", " ");
        config_save(nuevoFCB);
        free(nuevoFCB);
        return true;
    }    
}

int abrir_archivo(char* nombre){
    int tamRuta=strlen(path_fcb)+strlen(nombre)+5;
    char* ruta = (char*)malloc(tamRuta);
    strcpy(ruta,path_fcb);
    strcat(ruta,nombre);
    strcat(ruta,".fcb");
    config = iniciar_config(ruta);
    if(config!=NULL){
    int tamano=config_get_int_value(config,"TAMANIO_ARCHIVO");
    config_destroy(config);
    return tamano;
    }
    config_destroy(config);
    return -1;
}

void actualizarFcb(char* nombre, int tamano, int bloque){
    int tamRuta=strlen(path_fcb)+strlen(nombre)+5;
    char* ruta = (char*)malloc(tamRuta);
    strcpy(ruta,path_fcb);
    strcat(ruta,nombre);
    strcat(ruta,".fcb");   
    char str2[8];
    snprintf(str2, sizeof(str2), "%d", tamano);
    char str3[5];
    snprintf(str3, sizeof(str3), "%d", bloque);
    config = iniciar_config(ruta);
    free(ruta);
    if(config!=NULL){
    config_set_value(config,"TAMANIO_ARCHIVO",str2);
    config_set_value(config,"BLOQUE_INICIAL",str3);
    config_save(config);
    }
}

int obtener_bloqueInicial(char* nombre){
    int tamRuta=strlen(path_fcb)+strlen(nombre)+5;
    char* ruta = (char*)malloc(tamRuta);
    strcpy(ruta,path_fcb);
    strcat(ruta,nombre);
    strcat(ruta,".fcb");
    config = iniciar_config(ruta);
    free(ruta);
    if(config!=NULL){
    int bloque=config_get_int_value(config,"BLOQUE_INICIAL");
    return bloque;
    }
    return -1;
}
uint32_t buscarBloqueLibre(bool* bitmap){
    uint32_t i=1;//Arranco en 1.El bloque 0 es para boot.
    while(i<(cant_bloques_total-cant_bloques_swap) && bitmap[i]!=0)
    {
        i++;
    }
    return i;
}
//--------------------------------GESTION FAT---------------------------------
uint32_t buscarUltimoBloque(FILE* fat, int inicio, uint32_t* anteultimo){
    fseek(fat,sizeof(uint32_t)*inicio, SEEK_SET);
    uint32_t siguiente;
    uint32_t bloqueSig;
    sleep(retardo_acceso_fat/1000);
    fread(&siguiente,sizeof(uint32_t),1,fat);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",((ftell(fat)-sizeof(uint32_t))/sizeof(uint32_t)));
    bloqueSig=((ftell(fat)-sizeof(uint32_t))/sizeof(uint32_t));
    while(siguiente!=__UINT32_MAX__){
        bloqueSig=(ftell(fat)-sizeof(uint32_t))/sizeof(uint32_t);
        fseek(fat,siguiente,SEEK_SET);
        sleep(retardo_acceso_fat/1000);
        fread(&siguiente,sizeof(uint32_t),1,fat);
        if(siguiente==__UINT32_MAX__){
            anteultimo=bloqueSig;
            log_info(logger_filesystem,"ANTEULTIMO %d",anteultimo);
            bloqueSig=((ftell(fat)-sizeof(uint32_t))/sizeof(uint32_t));    
            log_info(logger_filesystem,"ULTIMO %d",bloqueSig);}
            log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",((ftell(fat)-sizeof(uint32_t))/sizeof(uint32_t)));
    }
    return bloqueSig;
}

void actualizarFAT(FILE* f, uint32_t ultimoBloque, uint32_t nuevoBloque){
    uint32_t direccionNuevo=nuevoBloque*sizeof(uint32_t);
    fseek(f,ultimoBloque*sizeof(uint32_t),SEEK_SET);
    fwrite(&direccionNuevo,sizeof(uint32_t),1,f);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",(ftell(f)-sizeof(uint32_t))/sizeof(uint32_t));
    fseek(f,direccionNuevo,SEEK_SET);
    uint32_t eof=__UINT32_MAX__;
    fwrite(&eof,sizeof(uint32_t),1,f);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",(ftell(f)-sizeof(uint32_t))/sizeof(uint32_t));
}

void ampliarArchivo(int bloqueInicio,int tamanoActual, int tamanoNuevo,bool* bitmap){
    int bloquesAmpliar= (tamanoNuevo-tamanoActual)/tam_bloque;
    if(bloquesAmpliar<=bloquesLibres){
        FILE* bloques=fopen(path_bloques, "rb+");
        FILE* fat=fopen(path_fat,"rb+");
        uint32_t anteultimo;
        uint32_t ultimoBloque=buscarUltimoBloque(fat, bloqueInicio,&anteultimo);
        uint32_t bloqueLibre=buscarBloqueLibre(bitmap);
        for(int i=0;i<bloquesAmpliar;i++){
            reservarBloque(bloques,bloqueLibre,bitmap);
            actualizarFAT(fat,ultimoBloque,bloqueLibre);
            ultimoBloque=buscarUltimoBloque(fat,bloqueInicio,&anteultimo);
            bloqueLibre=buscarBloqueLibre(bitmap);
        }
        fclose(fat);
        fclose(bloques);
    }
}

void achicarArchivo(int bloqueInicio, int tamanoActual, int tamanoNuevo, bool* bitmap){
    FILE* bloques = fopen(path_bloques, "rb+");
    FILE* fat = fopen(path_fat, "rb+");
    
    int bloquesLiberar = (tamanoActual - tamanoNuevo) / tam_bloque;
    log_info(logger_filesystem,"Debo liberar %d bloques",bloquesLiberar);
    uint32_t anteultimo;
    uint32_t libre = 0;
    uint32_t eof = __UINT32_MAX__;
    uint32_t ultimo;
    for(int i = 0; i < bloquesLiberar; i++){
        ultimo = buscarUltimoBloque(fat, bloqueInicio, &anteultimo);
        liberarBloque(bloques, ultimo, bitmap);
        fseek(fat, ultimo*sizeof(uint32_t), SEEK_SET);
        fwrite(&libre, sizeof(uint32_t), 1, fat);
        log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", (ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t));
        fseek(fat, anteultimo * sizeof(uint32_t), SEEK_SET);
        fwrite(&eof,sizeof(uint32_t), 1, fat);
        log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", (ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t));
        log_info(logger_filesystem, "liberando bloque %d", ultimo);
    }
    
    fclose(fat);
    fclose(bloques);
}

bool truncarArchivo(char* nombre, int tamano, bool* bitmap){
    int bloqueInicio=obtener_bloqueInicial(nombre);
    int tamanoActual=abrir_archivo(nombre);
    if(tamanoActual<tamano){
        log_info(logger_filesystem,"Debo ampliar %d bloques", (tamano-tamanoActual)/tam_bloque);
        ampliarArchivo(bloqueInicio,tamanoActual , tamano,bitmap);
        actualizarFcb(nombre,tamano,bloqueInicio);
        return true;
    }else if(tamanoActual>tamano){
        log_info(logger_filesystem,"Debo achicar");
        achicarArchivo(bloqueInicio,tamanoActual,tamano,bitmap);
        actualizarFcb(nombre, tamano,bloqueInicio);
        return true;
    }
    return false;
}

uint32_t asignarBloque(char* nombre, bool* bitmap){
    uint32_t bloque= buscarBloqueLibre(bitmap);
    FILE* bloques= fopen(path_bloques,"rb+");
    reservarBloque(bloques, bloque, bitmap);
    fclose(bloques);
    uint32_t eof=__UINT32_MAX__;
    FILE* fat= fopen(path_fat,"rb+");
    fseek(fat,(bloque)*sizeof(uint32_t),SEEK_SET);
    fwrite(&eof,sizeof(uint32_t),1,fat);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",(ftell(fat)-sizeof(uint32_t))/sizeof(uint32_t));
    fclose(fat);
    actualizarFcb(nombre,1024,bloque);
    return bloque;
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
        fwrite(&entrada,sizeof(uint32_t),1,f);
        //log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",(ftell(f)-sizeof(uint32_t))/sizeof(uint32_t));
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

void inicializarBitMap(bool* bitmap, int tamano){
    for(int i=0;i<tamano;i++)
    {
        bitmap[i]=0;
    }
    return;
}
//-------------------------------MAIN----------------------------------------------
int main(int argc, char* argv[]) {
    levantar_config("filesystem.config");
    int fd_filesystem = iniciar_servidor(logger_filesystem,NULL,puerto_escucha,"FILESYSTEM");
    int tamano_fat=(cant_bloques_total-cant_bloques_swap)*sizeof(uint32_t);
    bool* bitmapBloquesSwap=(bool*)malloc(sizeof(bool) * cant_bloques_swap);
    bool* bitmapBloques=(bool*)malloc(sizeof(bool) * (cant_bloques_total-cant_bloques_swap));
    inicializarBitMap(bitmapBloquesSwap, cant_bloques_swap);
    inicializarBitMap(bitmapBloques,(cant_bloques_total-cant_bloques_swap));
    if(!iniciar_fat(tamano_fat,path_fat)){
        log_error(logger_filesystem, "Error al crear el archivo FAT");
    }else{log_info(logger_filesystem,"FAT iniciada correctamente");}
    if(!iniciar_bloques(tam_bloque,bitmapBloques,bitmapBloquesSwap)){
        log_error(logger_filesystem, "Error al iniciar el archivo de bloques");
    }else{
        log_info(logger_filesystem,"Archivo de bloques iniciado correctamente: SWAP LIBRE %d, BLOQUES LIBRES %d",swapLibres,bloquesLibres);
    }
    bloquesOcupados(bitmapBloques);
    //Pruebas
    if(crear_archivo("probando2")){
    asignarBloque("probando2", bitmapBloques);}
    truncarArchivo("probando2", 1024*9,bitmapBloques);
    log_info(logger_filesystem,"Trunqué el archivo");
    int tamano=abrir_archivo("probando2");
    int bloque=obtener_bloqueInicial("probando2");
    actualizarFcb("probando2", tamano, bloque);
    log_info(logger_filesystem,"Archivo truncado: SWAP LIBRE %d, BLOQUES LIBRES %d",swapLibres,bloquesLibres);
    log_info(logger_filesystem,"Abri el archivo de tamano %d que inicia en el bloque %d", tamano, bloque);
    bloquesOcupados(bitmapBloques);
    truncarArchivo("probando2", 1024*5, bitmapBloques);
    tamano=abrir_archivo("probando2");
    bloque=obtener_bloqueInicial("probando2");
    actualizarFcb("probando2", tamano, bloque);
    bloquesOcupados;
    log_info(logger_filesystem,"Archivo truncado: SWAP LIBRE %d, BLOQUES LIBRES %d",swapLibres,bloquesLibres);
    log_info(logger_filesystem,"Abri el archivo de tamano %d que inicia en el bloque %d", tamano, bloque);
    
    //espero clientes kernel y memoria
 bloquesOcupados(bitmapBloques);
    while(server_escuchar_filesystem(logger_filesystem,"FILESYSTEM",fd_filesystem,bitmapBloques,bitmapBloquesSwap));

    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_filesystem, config);
    liberar_conexion(conexion_filesystem_memoria);
}