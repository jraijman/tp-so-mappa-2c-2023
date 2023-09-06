#ifndef MAIN_H_
#define MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <utils/utils.h>
#include <sockets/sockets.h>
#include <comunicacion.h>

int conexion_filesystem_memoria;

char* ip_memoria;
char* puerto_memoria;
char* puerto_escucha;
char* path_fat;
char* path_bloques;
char* path_fcb;
char* cant_bloques_total;
char* cant_bloques_swap;
char* tam_bloque;
char* retardo_acceso_bloque;
char* retardo_acceso_fat;

t_log* logger_filesystem;
t_config* config;

#endif 