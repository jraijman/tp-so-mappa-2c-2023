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

typedef struct main
{
    uint32_t num_marco;
    uint32_t pid;
    bool ocupado;
}t_marco;

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
t_list* l_marco;
void liberar_marco(t_marco*);
t_marco* marco_create(uint32_t, uint32_t,bool);
int reservar_primer_marco_libre(int);
void eliminar_proceso_memoria(int);
int calcularMarco(int pid, t_marco* marcos, int num_marcos);
#endif 