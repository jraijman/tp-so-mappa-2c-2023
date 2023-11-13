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

typedef struct{
	char* recurso;
	int instancias;
    int encontrado;
	t_list* procesos;
	t_queue* bloqueados;
	pthread_mutex_t mutex;
}t_recurso;



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
char** recursos;
int* instancia_recursos;

t_log* logger_kernel;
t_config* config;

//listas de estados
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_exec;
t_queue* cola_block;
t_queue* cola_exit;


//lista recursos
t_list *lista_recursos;

//semaforos
sem_t cantidad_multiprogramacion;
sem_t cantidad_new;
sem_t cantidad_ready;
sem_t cantidad_exit;
sem_t cantidad_exec;
sem_t cantidad_block;
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
int contador_pid = 0;

bool generar_conexiones();

void* leer_consola(void * arg);
void levantar_config(char* ruta);
t_list* config_list_to_t_list(t_config* config, char* nombre);
void iniciar_proceso(char*, char*, char*);
void finalizar_proceso(char*);
void detener_planificacion(void);
void iniciar_planificacion(void);
void cambiar_multiprogramacion(char* nuevo);
void proceso_estado(void);

void iniciar_listas();
void iniciar_hilos();
void iniciar_semaforos();

void agregar_a_new(pcb* proceso);
pcb* sacar_de_new();
void agregar_a_ready(pcb* proceso);
void agregar_a_exit(pcb* proceso);
void agregar_a_exec(pcb* proceso);
pcb* sacar_de_exec();
void * pasar_new_a_ready(void * args);
void * planif_corto_plazo(void* args);
void * planif_largo_plazo(void* args);
void manejar_recibir(int socket_fd);
pcb* obtenerSiguienteFIFO();
pcb* obtenerSiguientePRIORIDADES();
pcb* obtenerSiguienteRR();
void cambiar_estado(pcb *pcb, estado_proceso nuevo_estado);

pcb* crear_pcb(char* nombre_archivo, char * size, char * prioridad);
t_list* pid_lista_ready (t_list* lista);
char *estado_proceso_a_char(estado_proceso numero);
pcb* buscar_y_remover_pcb_cola(t_queue* cola, int id, sem_t s, pthread_mutex_t m);
t_list* inicializar_recursos();
int* string_to_int_array(char** array_de_strings);
t_recurso* buscar_recurso(char* recurso);
bool lista_contiene_id(t_list* lista, int id);


#endif 