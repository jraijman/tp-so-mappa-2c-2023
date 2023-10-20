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
    int tam_pagina;
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


typedef enum {
   ENVIO_PCB,
   ENVIO_INSTRUCCION,
   ENVIO_MARCO,
   ESTRUCTURAS_EN_MEMORIA_CONFIRMADO
} op_code;

///
bool send_int(int fd,int pid);
bool recv_int(int fd, int* pid);
bool send_pcb(int fd,pcb* proceso);
bool recv_pcb(int fd,pcb* proceso);
bool send_pcbDesalojado(pcbDesalojado proceso, int fd);
bool recv_pcbDesalojado(int fd, pcbDesalojado* proceso);
bool send_instruccion(int fd, Instruccion instruccion);
bool recv_instruccion(int fd, Instruccion* instruccion);
#endif 