#ifndef MAIN_H_
#define MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "comunicacion.h"

t_log* logger_filesystem;
t_config* config;
int swapLibre;
int bloqueLibre;

typedef struct {
    char* nombre_archivo;
    int tamanio_archivo;
    int bloque_inicial;
} FCB;
typedef struct {
    uint32_t entrada_FAT;
} FAT;
typedef struct {
    char* info;
} BLOQUE;

bool liberar_bloquesSWAP(int bloques[],int cantidad);
bool reservar_bloquesSWAP(int cant_bloques, int bloques_reservados[]);
int abrir_archivo(char* ruta);
void crear_archivo(char* nombre);
truncarArchivo(char* nombre, int tamano);
#endif