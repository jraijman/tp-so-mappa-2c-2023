#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include "../utils/utils.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/collections/list.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

typedef enum
{
    ENVIO_PCB,
    ENVIO_INSTRUCCION,
    ENVIO_PCB_DESALOJADO,
    ENVIO_DIRECCION,
    ENVIO_LISTA_ARCHIVOS,
    CAMBIAR_ESTADO,
    MANEJAR_IO,
    MANEJAR_WAIT,
    MANEJAR_SIGNAL,
    INICIALIZAR_PROCESO,
    FINALIZAR_PROCESO,
    PAQUETE,
    MENSAJE
} op_code;
typedef struct
{
    int size;
    void *stream;
} t_buffer;

typedef struct
{
    op_code codigo_operacion;
    t_buffer *buffer;
} t_paquete;

typedef enum
{
    NEW,
    READY,
    EXEC,
    BLOCK,
    EXIT_ESTADO,
} estado_proceso;

typedef struct
{
    int* ax;
    int* bx;
    int* cx;
    int* dx;
} t_registros;

typedef struct {
    char *path;
    int* puntero;
} t_archivos;

// Definición de estructura para representar un proceso (PCB)
typedef struct
{
    int pid; // Identificador del proceso
    int pc;  // Número de la próxima instrucción a ejecutar.
    int size;
    // valores de los registros de uso general de la CPU.
    int prioridad;    // Prioridad del proceso
    int tiempo_ejecucion;
    char *path;
    estado_proceso estado;
    t_registros *registros;
    t_list *archivos; // lista de archivos abiertos del proceso con la posición del puntero de cada uno de ellos
} pcb;

typedef enum
{
    SET,
    ADD,
    MOV_IN,
    MOV_OUT,
    IO,
    SIGNAL,
    EXIT,
    UNKNOWN_OP,
    ERROR_MEMORIA,
    WAIT,
    F_OPEN,
    YIELD,
    F_TRUNCATE,
    F_SEEK,
    CREATE_SEGMENT,
    F_WRITE,
    F_READ,
    DELETE_SEGMENT,
    F_CLOSE
} cod_instruccion;

typedef struct
{
    char *opcode; // Código de operación (por ejemplo, "SUM", "SUB", "SET", "EXIT")
    char *operando1;
    char *operando2;
} Instruccion;

// MOTIVOS DE BLOQUEO POSIBLES
typedef enum
{
    IO_BLOCK,
} motivo_block;

// MOTIVOS DE EXIT
typedef enum
{
    SUCCESS,
    SEG_FAULT,
    OUT_OF_MEMORY,
    RECURSO_INEXISTENTE,
} motivo_exit;

typedef struct
{
    pcb* contexto;
    char *instruccion;
    char *extra;
} pcbDesalojado;

typedef struct main
{
    uint32_t num_marco;
    uint32_t pid;
    bool ocupado;
} t_marco;
typedef struct
{
    int pid;              // PID del proceso que solicita la reserva
    int cantidad_bloques; // Cantidad de bloques de SWAP solicitados
} SolicitudReservaSwap;

typedef struct
{
    int pid;
    int cantidad_paginas;
    int *paginas;
} SolicitudLiberacionSwap;



typedef struct
{
    uint32_t direccionLogica;
    uint32_t tamano_pagina;
    uint32_t desplazamiento;
    uint32_t numeroMarco;
    uint32_t direccionFisica;
} Direccion;

///
typedef struct
{
    int tamano;
    int tipo;
} Recibido;

void pcb_destroyer(pcb* contexto);


//Mensajes
void enviar_mensaje(char* mensaje, int socket_cliente);
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
void recibir_mensaje(t_log* logger, int socket_cliente);
void crear_buffer(t_paquete* paquete);

//Paquetes
t_list* recibir_paquete(int);
t_paquete* crear_paquete(op_code);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void* serializar_paquete(t_paquete* paquete, int bytes);


//Empaquetados
void empaquetar_archivos(t_paquete* paquete_archivos, t_list* lista_archivos);
t_list* desempaquetar_archivos(t_list* paquete, int comienzo);
void empaquetar_pcb(t_paquete* paquete, pcb* contexto);
pcb* desempaquetar_pcb(t_list* paquete);
void empaquetar_registros(t_paquete* paquete, t_registros* registros);
t_registros* desempaquetar_registros(t_list* paquete, int comienzo);
void empaquetar_instruccion(t_paquete* paquete, Instruccion instruccion);
Instruccion desempaquetar_instruccion(t_list* paquete);


//send
void send_archivos(int fd_modulo,t_list* lista_archivos);
void send_pcb(pcb* contexto, int fd_modulo);
void send_cambiar_estado(estado_proceso estado, int fd_modulo);
void send_tiempo_io(int tiempo_io, int fd_modulo);
void send_recurso_wait(char* recurso, int fd_modulo);
void send_recurso_signal(char* recurso, int fd_modulo);
void send_inicializar_proceso(pcb * contexto, int fd_modulo);
void send_terminar_proceso(int pid, int fd_modulo);
void send_instruccion(int socket_cliente, Instruccion instruccion);

//recv
t_list* recv_archivos(t_log* logger, int fd_modulo);
pcb* recv_pcb(int fd_modulo);
estado_proceso recv_cambiar_estado(int fd_modulo);
int recv_tiempo_io(int fd_modulo);
char* recv_recurso(int fd_modulo);
int recv_terminar_proceso(int fd_modulo);
Instruccion recv_instruccion(int socket_cliente); 
// bool send_int(int fd, int pid);
// bool recv_int(int fd, int *pid);
// bool send_pcb(int fd, pcb *proceso);
// bool recv_pcb(int fd, pcb *proceso);
// // void deserializar_pcbDesalojado(void* stream, pcbDesalojado* proceso);
// // void* serializar_pcbDesalojado(pcbDesalojado proceso);
// bool send_pcbDesalojado(pcbDesalojado proceso, int fd);
// bool recv_pcbDesalojado(int fd, pcbDesalojado *proceso);
// bool send_instruccion(int fd, Instruccion instruccion);
// bool recv_instruccion(int fd, Instruccion *instruccion);
// bool send_direccion(int fd, Direccion *direccion);
// bool recv_direccion(int fd, Direccion *direccion);

#endif