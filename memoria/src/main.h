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

int conexion_memoria_filesystem;

char* puerto_escucha;
char* ip_filesystem;
char* puerto_filesystem;
char* tam_memoria;
char* tam_pagina;
char* path_instrucciones;
char* retardo_respuesta;
char* algoritmo_reemplazo;

t_log* logger_memoria;
t_config* config;

#endif 