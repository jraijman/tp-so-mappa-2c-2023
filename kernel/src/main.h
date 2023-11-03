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



int fd_cpu_dispatch;
int fd_cpu_interrupt;
int fd_memoria;
int fd_filesystem;
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
t_list* recursos;
t_list* instancia_recursos;

t_log* logger_kernel;
t_config* config;

int pid_a_eliminar = -1;
bool encontre_pid = false;

//listas de estados
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_exec;
t_queue* cola_block;
t_queue* cola_exit;

//semaforos
sem_t cantidad_multiprogramacion;
sem_t cantidad_new;
sem_t cantidad_ready;
sem_t cantidad_exit;
sem_t cantidad_exec;
sem_t puedo_ejecutar_proceso;

pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_block;
pthread_mutex_t mutex_exit;

// hilos
pthread_t hilo_consola;
pthread_t hilo_new_ready;
pthread_t hilo_plan_largo;
pthread_t hilo_cpu_exit;
pthread_t hilo_plan_corto;

// contador para id de procesos unico
int contador_proceso = 0;

bool generar_conexiones();

void* leer_consola(void * arg);
void levantar_config(char* ruta);
t_list* config_list_to_t_list(t_config* config, char* nombre);
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
void * planif_corto_plazo(void* args);
void * planif_largo_plazo(void* args);
pcb* obtenerSiguienteFIFO();
pcb* obtenerSiguientePRIORIDADES();
pcb* obtenerSiguienteRR();
void cambiar_estado(pcb *pcb, estado_proceso nuevo_estado);

t_list* pid_lista_ready (t_list* lista);
char *estado_proceso_a_char(estado_proceso numero);

#endif 