#ifndef MAIN_H_
#define MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "comunicacion.h"



int conexion_dispatch;
int conexion_interrupt;
int conexion_memoria;
int conexion_fileSystem;
char* ip_memoria;
char* ip_cpu;
char* puerto_memoria;
char* ip_filesystem;
char* puerto_filesystem;
char* puerto_cpu_interrupt;
char* puerto_cpu_dispatch;
char* algoritmo_planificacion;
char* quantum;
int grado_multiprogramacion;
char* recursos;
char* instancia_recursos;

t_log* logger_kernel;
t_config* config;

//listas de estados
t_queue* cola_new;
t_list* lista_ready;
t_list* lista_exec;
t_list* lista_block;
t_list* lista_exit;

//semaforos
sem_t cantidad_multiprogramacion;
sem_t cantidad_new;
sem_t cantidad_ready;

pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_block;
pthread_mutex_t mutex_exit;

// hilos
pthread_t hilo_consola;
pthread_t hilo_new_ready;
pthread_t hilo_cpu_exit;

// contador para id de procesos unico
int contador_proceso = 0;


void* leer_consola(void * arg);
void levantar_config(char* ruta);
void iniciar_proceso(char*, char*, char*);
void finalizar_proceso(char*);
void detener_planificacion(void);
void iniciar_planificacion(void);
void multiprogramacion(/*char**/);
void proceso_estado(void);

void iniciar_listas();
void iniciar_hilos();
void iniciar_semaforos();

void agregar_a_new(pcb* proceso);
pcb* sacar_de_new();
void agregar_a_ready(pcb* proceso);
void * pasar_new_a_ready(void * args);
void * finalizar_proceso_cpu(void * args);


char* pid_lista_ready (t_list* lista);


#endif 