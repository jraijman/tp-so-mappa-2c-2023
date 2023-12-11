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
#include "../../utils/src/protocolo/protocolo.h"
#include <pthread.h>
#include <semaphore.h>

int fd_memoria;

t_list* paginas_en_memoria;



// Memoria fisica
uint8_t* bitarray_marcos_ocupados;
void* memoria;

//semaforos
sem_t swap_asignado;
pthread_mutex_t mx_memoria;
pthread_mutex_t mx_bitarray_marcos_ocupados;

char* server_name;

int conexion_memoria_filesystem;
char* puerto_escucha;
char* ip_filesystem;
char* puerto_filesystem;
int tam_memoria;
int tam_pagina;
char* path_instrucciones;
int RETARDO_REPUESTA;
char* algoritmo_reemplazo;
void *espacio_usuario;

t_log* logger_obligatorio;
t_log* logger_memoria;
t_config* config;
t_list* l_marco;
t_list* l_proceso;
void inicializar_memoria();
void liberar_marco(t_marco*);
t_marco* marco_create(uint32_t, uint32_t,bool);
int reservar_primer_marco_libre(int);
void eliminar_proceso_memoria(int);
int calcularMarco(int pid, t_marco* marcos, int num_marcos);
int obtenerCantidadPaginasAsignadas(int pid);
int server_escuchar(int fd_memoria);
void terminar_proceso(int pid);
t_list* inicializar_proceso(pcb* pcb, t_list* bloques);
pcb* encontrar_proceso(int pid);
void* obtener_marco(uint32_t nro_marco); 
void eliminar_proceso(int pid);
void liberar_recursos(pcb* proceso);
void escribir_marco_en_memoria(uint32_t nro_marco, uint32_t* marco);
int get_memory_and_page_size();
int buscar_marco_libre();
t_list* obtenerMarcosAsignados( int pid);
int paginas_necesarias(pcb *proceso);
Instruccion armar_estructura_instruccion(char* instruccion_leida);
char *armar_path_instruccion(char *path_consola);
int usar_algoritmo(int pid, int num_pagina);
int algoritmo_fifo(int pid, int num_pagina);
int algoritmo_lru(int pid, int num_pagina);
bool compararTiempoUso(void *unaPag, void *otraPag);
void actualizarTiempoDeUso(t_list* paginas_en_memoria);
void leer_instruccion_por_pc_y_enviar(char *path_consola, int pc, int fd);
void eliminar_tabla_paginas(int pid);
void log_valor_espacio_usuario(char* valor, int tamanio);
int obtener_nro_marco_memoria(int num_pagina, int pid_actual);
int tratar_page_fault(int num_pagina, int pid_actual);
void escribir_bloque_en_memoria(char* bloque_swap, int nro_marco);
char* leer_marco_de_memoria(int nro_marco);
uint32_t* leer_registro_de_memoria_uint(int nro_marco);
entrada_pagina * buscar_en_tabla_por_direccionfisica(int direccionfisica);
void marcar_pagina_modificada(int dirFisica);



#endif 