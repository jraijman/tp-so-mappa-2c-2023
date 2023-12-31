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
    HAY_ALGO_MAL,
    ENVIO_PCB,
    ENVIO_INSTRUCCION,
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
    PEDIDO_MARCO,
    ENVIO_MARCO,
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
    ARCHIVO_EXISTE,
    ARCHIVO_NO_EXISTE,
    ARCHIVO_CREADO,
    TAMANIO_PAGINA,
    CARGAR_PAGINA,
    PAGINA_CARGADA,
    PEDIDO_SWAP,
    ESCRIBIR_SWAP,
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
	//int puntero;  //no se usa, lo tiene cada proceso
    char* modo_apertura;
	t_queue* bloqueados_archivo;//lista de t_proceso_bloqueado_archivo
	pthread_mutex_t mutex_archivo;
    int abierto_w;
    int cant_abierto_r;
} t_archivo;

typedef struct {
    char* nombre_archivo;
	int puntero;
    char* modo_apertura;
} t_archivo_proceso;

typedef struct {
    int pid;
    char* modo_apertura;
} t_proceso_bloqueado_archivo;



t_list* lista_tablas_de_procesos;
typedef struct 
{
    int num_marco;
    time_t ultimo_tiempo_uso;
    bool en_memoria; //bit de presencia
    bool modificado; // bit de modificado
    int pid;
    int posicion_swap; 
    int num_pagina;
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



typedef struct {
    int marco;
    int desplazamiento;
} DireccionFisica;

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
char* recibir_mensaje_fs(int socket_cliente);
void enviar_mensaje(char* mensaje, int socket_cliente);
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
void recibir_mensaje(t_log* logger, int socket_cliente);
void crear_buffer(t_paquete* paquete);
void send_liberacion_swap(int fd,int cantidad, int bloques[]);
void send_fin_escritura(int fd_modulo);
void send_pcbDesalojado(pcb* contexto, char* instruccion, char* extra, int fd, t_log* logger);
void recv_pcbDesalojado(int fd,pcb** contexto, char** extra);
//Paquetes
t_list* recibir_paquete(int);
t_paquete* crear_paquete(op_code);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void* serializar_paquete(t_paquete* paquete, int bytes);
void registros_destroy(t_registros* registros);

//Empaquetados
void empaquetar_archivos(t_paquete* paquete_archivos, t_list* lista_archivos);
t_list* desempaquetar_archivos(t_list* paquete, int* comienzo);
void empaquetar_pcb(t_paquete* paquete, pcb* contexto);
pcb* desempaquetar_pcb(t_list* paquete, int* counter);
void empaquetar_registros(t_paquete* paquete, t_registros* registros);
t_registros* desempaquetar_registros(t_list* paquete, int comienzo);
void empaquetar_instruccion(t_paquete* paquete, Instruccion instruccion);
Instruccion desempaquetar_instruccion(t_list* paquete);
t_list* recv_bloques_reservados(int fd_modulo);


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
void send_tam_pagina(int tam, int fd_modulo);
void send_pagina_cargada(int fd);
void send_pedido_swap(int fd, int posicion_swap);
void send_leido_swap(int fd, char * leido,int tam_pagina);
void send_cargar_pagina(int fd_modulo, int pid, int pagina);
void send_leer_archivo(int fd_modulo, char* nombre_archivo, DireccionFisica direccion_fisica, int puntero);
void send_escribir_archivo(int fd_modulo, char* nombre_archivo, DireccionFisica direccion_fisica, int puntero);

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
void recv_f_open(int fd,char** nombre_archivo, char ** modo_apertura, pcb ** contexto);
void recv_f_close(int fd,char** nombre_archivo, pcb ** contexto);
t_list* recv_reserva_swap(int fd_modulo);
int recv_tam_pagina(int fd_modulo);
char * recv_leido_swap(int fd_modulo);
void recv_cargar_pagina(int fd_modulo, int *pid, int *pagina);

void send_pedido_marco(int fd_modulo, int pid, int pagina);
void recv_pedido_marco(int fd_modulo, int *pid, int *pagina);
void send_marco(int fd_modulo, int memoria_fisica);
int recv_marco(int fd_modulo);
void send_pcb_page_fault(int fd_modulo, pcb* contexto, int pagina);
void recv_pcb_page_fault(int fd_modulo, pcb** contexto, int *pagina);
char *recv_pagina_cargada(int fd_modulo);

int recv_respuesta_abrir_archivo(int fd_filesystem);
void recv_f_truncate(int fd,char** nombre_archivo, int* tam, pcb** contexto);
void recv_f_write(int fd,char** nombre_archivo, DireccionFisica* direccion, pcb** contexto);

#endif