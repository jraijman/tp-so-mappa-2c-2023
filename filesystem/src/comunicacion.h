#ifndef COMUNICACION_H_
#define COMUNICACION_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "../../utils/src/protocolo/protocolo.h"
#include "main.h"

t_log* logger_filesystem;
t_config* config;
int swapLibres;
int bloquesLibres;
char* ip_memoria;
char* puerto_memoria;
char* puerto_escucha;
char* path_fat;
char* path_bloques;
char* path_fcb;
int cant_bloques_total;
int cant_bloques_swap;
int tam_bloque;
int retardo_acceso_bloque;
int retardo_acceso_fat;

int conexion_filesystem_memoria;


typedef struct {
    t_log* log;
    int fd;
    char* server_name;
    bool* bitmapBloques;
    bool* bitmapSwap;
} t_procesar_conexion_args;

typedef struct {
    int fd;
    int num_bloque;
} t_pedido_swap_args;
 typedef struct {
    int fd;
    int num_bloque;
    char* info_a_escribir;
} t_pedido_escribir_swap_args;

 typedef struct {
    int fd;
    int cantidad_bloques;
    bool* bitmapSwap;
    int* bloques_a_liberar;
} t_finalizar_proceso_args;

 typedef struct {
    int fd;
    DireccionFisica direccion;
    int puntero;
    char* nombre;
} t_proceso_args;

 typedef struct {
    int fd;
    char* nombre;
    int tamanio;
    bool* bitmapBloques;
} t_truncar_args;


int server_escuchar_filesystem(t_log* logger,char* server_name,int server_socket,bool* bitmapBloques, bool* bitmapSwap);

void* manejar_iniciar_proceso(void* arg);
void* manejar_pedido_swap(void* arg);
void* manejar_escribir_swap(void* arg);
void* manejar_finalizar_proceso(void* arg);
void* manejar_truncar(void* arg);
void* manejar_write_proceso(void* args);
void* manejar_read_proceso(void* args);


#endif 