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

uint32_t handshake = 1;
uint32_t result;

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
t_queue* colaNew;
t_list* listaReady;
t_list* listaExec;
t_list* listaBlock;
t_list* listaExit;

//semaforos
sem_t cola_ready;
sem_t cantidad_new;

// hilos
pthread_t hiloConsola;
pthread_t hilo_new_ready;

// Definición de estructura para representar un proceso (PCB)
typedef struct {
    int pid;            // Identificador del proceso
    int pc; // Número de la próxima instrucción a ejecutar.
    int size;
    struct Reg 
    {
        uint32_t ax;
        uint32_t bx;
        uint32_t cx;
        uint32_t dx;
    } registros;  // valores de los registros de uso general de la CPU.
    int prioridad;      // Prioridad del proceso
    t_list*  archivos; // lista de archivos abiertos del proceso con la posición del puntero de cada uno de ellos
    int estado;    // Estado del proceso (  1= NEW, 2 = READY, 3= RUNNING, 4 =BLOCK, 5 = FINISH.)
} pcb;


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

void agregarNew(pcb* proceso);
void * pasarAReady(void * args);

#endif 