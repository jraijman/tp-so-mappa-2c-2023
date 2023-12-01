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
#endif