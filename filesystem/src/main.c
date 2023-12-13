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
            //usleep(retardo_acceso_bloque * 1000);
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
    long aux;
    if (f != NULL) {
        int j = 0;
        while (j < cant_bloques) {
            //sleep(retardo_acceso_bloque / 1000);
            //log_info(logger_filesystem, "ACCESO A BLOQUE SWAP NRO: %ld", (ftell(f) / tam_bloque));
            fread(bloque.info, tam_bloque, 1, f);
            fseek(f,-(tam_bloque),SEEK_CUR);
            if (strcmp(bloque.info, "0") == 0) {
                swapLibres--;
                strcpy(bloque.info, "\0");
                bloques_reservados[j] =(ftell(f) / tam_bloque);
                //sleep(retardo_acceso_bloque / 1000);
                bitmap[ftell(f) / tam_bloque] = 1;
                log_info(logger_filesystem, "BLOQUES RESERVADOS: %ld", (ftell(f) / tam_bloque));
                fwrite(bloque.info, tam_bloque, 1, f);
                j++;
            } else {
                fseek(f, tam_bloque, SEEK_CUR);
            }
        }
        fclose(f);
        free(bloque.info);
        return true;
    } else {
        free(bloque.info);
        return false;
    }
}

//-----------------------LEER BLOQUE-------------------------------------------
char* leer_bloque(int num_bloque){
	FILE* f=fopen(path_bloques, "rb");
    if(f!=NULL){
        char* info = malloc(tam_bloque);
        usleep(retardo_acceso_bloque * 1000);
        if(num_bloque<cant_bloques_swap){
            fseek(f, tam_bloque * num_bloque, SEEK_SET);
            log_info(logger_filesystem, "ACCESO A BLOQUE SWAP NRO: %ld",ftell(f) / tam_bloque);
            fread(info, tam_bloque,1,f);
            fclose(f);
        }else{
            fseek(f, tam_bloque * num_bloque, SEEK_SET);
            log_info(logger_filesystem, "ACCESO A BLOQUE NRO: %ld",ftell(f) / tam_bloque-cant_bloques_swap);
            fread(info, tam_bloque,1,f);
            fclose(f);
        }
        //log_info(logger_filesystem, "LA INFO LEIDA ES: %s", info);
        return info;
    }
    else{
        fclose(f);
        return NULL;
    }
}
char* escribir_bloque(int num_bloque, char* info){
	FILE* f=fopen(path_bloques, "rb+");
    if(f!=NULL){
        usleep(retardo_acceso_bloque * 1000);
        fseek(f, tam_bloque * num_bloque, SEEK_SET);
        if(num_bloque<cant_bloques_swap){
            log_info(logger_filesystem, "ACCESO A BLOQUE SWAP NRO: %ld",ftell(f) / tam_bloque);
            fwrite(info, tam_bloque,1,f);
        }else{
            usleep(retardo_acceso_bloque * 1000);
            fseek(f, tam_bloque * num_bloque, SEEK_SET);
        }
        /*else if(num_bloque<cant_bloques_swap){
        log_info(logger_filesystem, "ACCESO A BLOQUE %ld",ftell(f) / tam_bloque);
        fwrite(info, tam_bloque,1,f);   
        }*/
        //log_info(logger_filesystem, "LA INFO ESCRITA ES: %s", info);
        fclose(f);
        return info;
    }
    else{
        fclose(f);
        return NULL;
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
                //bitmapSwap[i] = 0;
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

void bloqueOcupado(bool* bitmap){
    for(int i=0;i<cant_bloques_total-cant_bloques_swap;i++)
    {
        if(bitmap[i]==1)
        {
            log_info(logger_filesystem,"EL BLOQUE %d ESTA OCUPADO",i);
        }
    }
}
void reservarBloque(FILE* f, uint32_t bloque, bool* bitmap) {
    bloque=bloque+cant_bloques_swap;
    fseek(f,bloque*tam_bloque, SEEK_SET);
    BLOQUE reservado;
    reservado.info = malloc(tam_bloque);
    strcpy(reservado.info, "\0");
    //usleep(retardo_acceso_bloque * 1000);
    log_info(logger_filesystem, "ACCESO A BLOQUE NRO: %ld", ftell(f)/tam_bloque-cant_bloques_swap);
    fwrite(reservado.info, tam_bloque, 1, f);
    bitmap[bloque-cant_bloques_swap] = 1;
    bloque=ftell(f)/tam_bloque;
    bloquesLibres--;    
    free(reservado.info);
}

void liberarBloque(FILE* f, uint32_t bloqueLib, bool* bitmap) {
    bloqueLib=bloqueLib+cant_bloques_swap;
    fseek(f, bloqueLib*tam_bloque, SEEK_SET);
    BLOQUE bloque;
    bloque.info = malloc(tam_bloque);
    strcpy(bloque.info, "0");
    //usleep(retardo_acceso_bloque * 1000);
    log_info(logger_filesystem, "ACCESO A BLOQUE NRO: %ld", ftell(f)/tam_bloque-cant_bloques_swap);
    fwrite(bloque.info, tam_bloque, 1, f);
    bitmap[bloqueLib-cant_bloques_swap] = 0;
    bloquesLibres++;
    free(bloque.info);
}

//----------------------------GESTION FCBs-------------------------------------
bool crear_archivo(char* nombre) {
    char nombreArchivo[strlen(nombre) + 1];
    strcpy(nombreArchivo, nombre);
    int tam = strlen(path_fcb) + strlen(nombre) + 6;
    char ruta[tam];
    strcpy(ruta, path_fcb);
    strcat(ruta,"/");
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
    int tamRuta=strlen(path_fcb)+strlen(nombre)+6;
    char* ruta = (char*)malloc(tamRuta);
    strcpy(ruta,path_fcb);
    strcat(ruta,"/");
    strcat(ruta,nombre);
    strcat(ruta,".fcb");
    config = iniciar_config(ruta);
    if(config!=NULL){
    int tamano=config_get_int_value(config,"TAMANIO_ARCHIVO");
    config_destroy(config);
    free(ruta);
    return tamano;
    }
    //config_destroy(config);
    free(ruta);
    return -1;
}

void actualizarFcb(char* nombre, int tamano, int bloque){
    int tamRuta=strlen(path_fcb)+strlen(nombre)+6;
    char* ruta = (char*)malloc(tamRuta);
    strcpy(ruta,path_fcb);
    strcat(ruta,"/");
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
    int tamRuta=strlen(path_fcb)+strlen(nombre)+6;
    char* ruta = (char*)malloc(tamRuta);
    strcpy(ruta,path_fcb);
    strcat(ruta,"/");
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
int obtener_bloque(int bloqueInicial,int nroBloque){
FILE* fat = fopen(path_fat,"rb");
fseek(fat,(sizeof(uint32_t))*bloqueInicial,SEEK_SET);
uint32_t siguiente;
for(int i=0; i<nroBloque-1; i++){
    if(siguiente==__UINT32_MAX__){
        fclose(fat);
        return -1;
    }
    sleep(retardo_acceso_fat / 1000);
    fread(&siguiente, sizeof(uint32_t), 1, fat);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", ((ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t)));
}
fclose(fat);
return siguiente;
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
uint32_t buscarUltimoBloque(FILE* fat, int inicio) {
    fseek(fat, sizeof(uint32_t) * inicio, SEEK_SET);
    uint32_t siguiente;
    uint32_t bloqueSig;
    sleep(retardo_acceso_fat / 1000);
    fread(&siguiente, sizeof(uint32_t), 1, fat);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", ((ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t)));
    bloqueSig = ((ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t));
    while (siguiente != __UINT32_MAX__) {
        bloqueSig = (ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t);
        fseek(fat, siguiente, SEEK_SET);
        sleep(retardo_acceso_fat / 1000);
        fread(&siguiente, sizeof(uint32_t), 1, fat);
        bloqueSig = (ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t);
        log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", (ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t));
    }
    return bloqueSig;
}

void actualizarFAT(FILE* f, uint32_t ultimoBloque, uint32_t nuevoBloque){
    uint32_t direccionNuevo=nuevoBloque*sizeof(uint32_t);
    fseek(f,ultimoBloque*sizeof(uint32_t),SEEK_SET);
    sleep(retardo_acceso_fat / 1000);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",(ftell(f))/sizeof(uint32_t));
    fwrite(&direccionNuevo,sizeof(uint32_t),1,f);
    fseek(f,direccionNuevo,SEEK_SET);
    uint32_t eof=__UINT32_MAX__;
    sleep(retardo_acceso_fat / 1000);
    log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld",(ftell(f))/sizeof(uint32_t));
    fwrite(&eof,sizeof(uint32_t),1,f);
}
void bloquesArchivo(FILE* fat,int inicio,int tamano,uint32_t* bloques){
    //no se como hacer aca 
    if(inicio == -1){
        return;
    }
    fseek(fat, sizeof(uint32_t) * inicio, SEEK_SET);
    uint32_t siguiente=0;
    uint32_t bloqueSig=0;
    int i=0;
    while (siguiente != __UINT32_MAX__) {
        bloques[i]=ftell(fat)/sizeof(uint32_t);
        usleep(retardo_acceso_fat * 1000);
        fread(&siguiente, sizeof(uint32_t), 1, fat);
        log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", ((ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t)));
        fseek(fat, siguiente * sizeof(uint32_t), SEEK_SET);
        i++;
    }
}

void ampliarArchivo(int bloqueInicio,int tamanoActual, int tamanoNuevo,bool* bitmap){

    int bloquesAmpliar= (tamanoNuevo-tamanoActual)/tam_bloque;
    if(bloquesAmpliar<=bloquesLibres){
        FILE* bloques=fopen(path_bloques, "rb+");
        FILE* fat=fopen(path_fat,"rb+");
        uint32_t libre=0;
        uint32_t eof=__UINT32_MAX__;
        uint32_t* bloquesOcupados = (uint32_t*)calloc(tamanoNuevo/tam_bloque,sizeof(uint32_t));
        bloquesOcupados[0]=bloqueInicio;
        uint32_t bloqueLibre;
        bloquesArchivo(fat,bloqueInicio,tamanoNuevo/tam_bloque,bloquesOcupados);
        for(int i=(tamanoActual/tam_bloque)-1;i<tamanoNuevo/tam_bloque-1;i++){
            bloqueLibre=buscarBloqueLibre(bitmap);
            bloquesOcupados[i+1]=bloqueLibre;
            actualizarFAT(fat,bloquesOcupados[i],bloquesOcupados[i+1]);
            reservarBloque(bloques,bloqueLibre,bitmap);
        }
        fclose(fat);
        fclose(bloques);
        free(bloquesOcupados);
    }
}
void achicarArchivo(int bloqueInicio, int tamanoActual, int tamanoNuevo, bool* bitmap){
    FILE* bloques = fopen(path_bloques, "rb+");
    FILE* fat = fopen(path_fat, "rb+");
    int bloquesLiberar=(tamanoActual-tamanoNuevo)/tam_bloque;
    uint32_t libre=0;
    uint32_t eof=__UINT32_MAX__;
    uint32_t* bloquesOcupados = (uint32_t*)calloc(tamanoActual / tam_bloque, sizeof(uint32_t));
    bloquesArchivo(fat,bloqueInicio,tamanoActual/tam_bloque,bloquesOcupados);
    for(int i = tamanoActual/tam_bloque; i > tamanoNuevo/tam_bloque; i--){
        liberarBloque(bloques, bloquesOcupados[i-1], bitmap);
        fseek(fat, bloquesOcupados[i-1]*sizeof(uint32_t), SEEK_SET);
        fwrite(&libre, sizeof(uint32_t), 1, fat);
        log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", (ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t));
        fseek(fat, bloquesOcupados[i-2] * sizeof(uint32_t), SEEK_SET);
        fwrite(&eof,sizeof(uint32_t), 1, fat);
        log_info(logger_filesystem, "ACCESO A FAT, ENTRADA %ld", (ftell(fat) - sizeof(uint32_t)) / sizeof(uint32_t));
    }
    fclose(fat);
    fclose(bloques);
    free(bloquesOcupados);
}


bool truncarArchivo(char* nombre, int tamano, bool* bitmap){
    int bloqueInicio=obtener_bloqueInicial(nombre);
    int tamanoActual=abrir_archivo(nombre);
    if(tamanoActual==0)
    {
        bloqueInicio=asignarBloque(nombre,bitmap);
    }
    if(tamanoActual<tamano){
        ampliarArchivo(bloqueInicio, tamanoActual, tamano, bitmap);
        actualizarFcb(nombre,tamano,bloqueInicio);
        return true;
    }else if(tamanoActual>tamano){
        achicarArchivo(bloqueInicio,tamanoActual,tamano,bitmap);
        actualizarFcb(nombre, tamano,bloqueInicio);
        return true;
    }
    return false;
}

int asignarBloque(char* nombre, bool* bitmap){
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
    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }
    
    // CONFIG y logger
    levantar_config(argv[1]);

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
    //Pruebas
    //espero clientes kernel y memoria
    while(server_escuchar_filesystem(logger_filesystem,"FILESYSTEM",fd_filesystem,bitmapBloques,bitmapBloquesSwap));

    //CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_filesystem, config);
    liberar_conexion(conexion_filesystem_memoria);
}