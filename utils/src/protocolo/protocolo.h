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
#include<commons/collections/queue.h>

typedef enum
{
    //OPCODES
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
    MENSAJE,
    PCB_SIGNAL,
    PCB_WAIT,
    PCB_SLEEP,
    PCB_INTERRUPCION,
    PCB_EXIT,
    INTERRUPCION,
    PCB_PAGEFAULT,
    CONEXION_MEMORIA,
    ABRIR_ARCHIVO,
    RESERVA_SWAP,
    BLOQUES_RESERVADOS,
    LIBERACION_SWAP,
    CREAR_ARCHIVO,
    TRUNCAR_ARCHIVO,
    PEDIDO_LECTURA_FS,
    PEDIDO_ESCRITURA_FS,
    FIN_ESCRITURA,
    INTERRUPCION_FINALIZAR,
    ARCHIVO_NO_EXISTE,
    ARCHIVO_CREADO,
    //INSTRUCCIONES
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
    uint32_t* ax;
    uint32_t* bx;
    uint32_t* cx;
    uint32_t* dx;
} t_registros;

typedef struct {
    char* nombre_archivo;
	int puntero;
	t_queue* bloqueados_archivo;
	//pthread_mutex_t mutex_w;
    //pthread_mutex_t mutex_r;
    //pthread_rwlock_t rwlock;
    int abierto_w;
    int cant_abierto_r;
} t_archivo;




t_list* lista_tablas_de_procesos;
typedef struct 
{
    int num_marco;
    int tiempo_uso;
    bool en_memoria; //bit de presencia
    bool modificado; // bit de modificado
    int pid;
    int posicion_swap; 
} entrada_pagina;

// Definición de estructura para representar un proceso (PCB)



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





typedef struct main
{
    uint32_t num_marco;
    uint32_t pid;
    bool ocupado;
} t_marco;

typedef struct
{
    int pid;
    int cantidad_paginas;
    int *paginas;
} SolicitudLiberacionSwap;



typedef struct
{
    int direccionLogica;
    int tamano_pagina;
    int desplazamiento;
    int numeroPagina;
    int direccionFisica;
    int marco;
    bool pageFault;
} Direccion;

///
typedef struct
{
    int tamano;
    int tipo;
} Recibido;

typedef struct
{
    int pid; // Identificador del proceso
    int pc;  // Número de la próxima instrucción a ejecutar.
    int size;
    int prioridad;    // Prioridad del proceso
    int tiempo_ejecucion;
    char *path;
    estado_proceso estado;
    t_registros *registros;
    t_list *archivos; 
} pcb;

void pcb_destroyer(pcb* contexto);


//Mensajes
void enviar_mensaje(char* mensaje, int socket_cliente);
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
void recibir_mensaje(t_log* logger, int socket_cliente);
void crear_buffer(t_paquete* paquete);
void send_pcbDesalojado(pcb* contexto, char* instruccion, char* extra, int fd, t_log* logger);
void recv_pcbDesalojado(int fd,pcb** contexto, char** extra);
void send_direccion(int conexion_cpu_memoria,Direccion* direccion);
void recv_direccion(int conexion_cpu_memoria,Direccion* direccion);
//Paquetes
t_list* recibir_paquete(int);
t_paquete* crear_paquete(op_code);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void* serializar_paquete(t_paquete* paquete, int bytes);


//Empaquetados
void empaquetar_archivos(t_paquete* paquete_archivos, t_list* lista_archivos);
t_list* desempaquetar_archivos(t_list* paquete, int* comienzo);
void empaquetar_pcb(t_paquete* paquete, pcb* contexto);
pcb* desempaquetar_pcb(t_list* paquete, int* counter);
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
void send_fetch_instruccion(char * path, int pc, int fd_modulo);
void send_pcbDesalojado(pcb* contexto, char* instruccion, char* extra, int fd, t_log* logger);
void send_valor_leido_fs(char* valor, int tamanio, int fd_modulo);
void send_escribir_valor_fs(char* valor, int dir_fisica, int tamanio, int pid, int fd_modulo);
void send_abrir_archivo(char* nombre_archivo, int fd_modulo);
void send_crear_archivo(char* nombre_archivo, int fd_modulo);
void send_reserva_swap(int fd, int cant_paginas_necesarias);

//recv
t_list* recv_archivos(t_log* logger, int fd_modulo);
pcb* recv_pcb(int fd_modulo);
estado_proceso recv_cambiar_estado(int fd_modulo);
int recv_tiempo_io(int fd_modulo);
char* recv_recurso(int fd_modulo);
int recv_terminar_proceso(int fd_modulo);
Instruccion recv_instruccion(int socket_cliente);
int recv_fetch_instruccion(int fd_modulo, char** path, int** pc);
void send_interrupcion(int pid, int fd_modulo);
int recv_interrupcion(int fd_modulo);
void recv_f_open(int fd,char** nombre_archivo, char ** modo_apertura);
void recv_f_close(int fd,char** nombre_archivo);
int recv_reserva_swap(int fd_modulo);


#endif