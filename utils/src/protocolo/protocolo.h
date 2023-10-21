#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "../utils/utils.h"
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>

#define NEW 1
#define READY 2
#define EXEC 3
#define BLOCK 4
#define EXIT 5
// Definición de estructura para representar un proceso (PCB)
typedef struct {
    int pid; // Identificador del proceso
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
    int estado;    // Estado del proceso (  1= NEW, 2 = READY, 3= EXEC, 4 =BLOCK, 5 = EXIT.)
    char path[256];
} pcb;
typedef struct {
    char opcode[11];   // Código de operación (por ejemplo, "SUM", "SUB", "SET", "EXIT")
    char operando1[25];
    char operando2[25];
} Instruccion;
typedef struct 
{
    pcb contexto;
    char* instruccion;
    char* extra;
}pcbDesalojado;

typedef struct main
{
    uint32_t num_marco;
    uint32_t pid;
    bool ocupado;
}t_marco;
typedef struct {
    int pid;              // PID del proceso que solicita la reserva
    int cantidad_bloques;  // Cantidad de bloques de SWAP solicitados
} SolicitudReservaSwap;
typedef struct {
    int pid;              
    int cantidad_paginas; 
    int* paginas;         
} SolicitudLiberacionSwap;

typedef enum{
    ENVIO_PCB,
    ENVIO_INSTRUCCION,
    ENVIO_PCB_DESALOJADO,
    ENVIO_DIRECCION,
}op_code;

typedef struct
{
    uint32_t direccionLogica;
    uint32_t tamano_pagina;
    uint32_t desplazamiento;
    uint32_t numeroMarco;
    uint32_t direccionFisica;
} Direccion;

///
typedef struct {
    int tamano;
    int tipo;
} Recibido;

bool send_int(int fd,int pid);
bool recv_int(int fd, int* pid);
bool send_pcb(int fd,pcb* proceso);
bool recv_pcb(int fd,pcb* proceso);
//void deserializar_pcbDesalojado(void* stream, pcbDesalojado* proceso);
//void* serializar_pcbDesalojado(pcbDesalojado proceso);
bool send_pcbDesalojado(pcbDesalojado proceso, int fd);
bool recv_pcbDesalojado(int fd, pcbDesalojado* proceso);
bool send_instruccion(int fd, Instruccion instruccion);
bool recv_instruccion(int fd, Instruccion* instruccion);
bool send_direccion(int fd, Direccion direccion);
bool recv_direccion(int fd, Direccion* direccion);

#endif 