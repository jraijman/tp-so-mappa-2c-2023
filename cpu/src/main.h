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
#include "../../utils/src/protocolo/protocolo.h"
#include "instrucciones.h"

bool recibio_interrupcion;
bool flag_ciclo;

int fd_cpu_dispatch;
int fd_cpu_interrupt;
int  conexion_cpu_memoria;
char* ip_memoria;
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
t_log* logger_cpu;
t_config* config;
pcb* contexto;

void levantar_config(char*);
void ciclo_instruccion(pcb* contexto,int cliente_socket_dispatch,int cliente_socket_interrupt, t_log* logger);
void decodeInstruccion(Instruccion* instruccion, pcb* contexto);
bool fetchInstruccion(int fd, pcb* contexto, Instruccion *instruccion, t_log* logger);
void executeInstruccion(pcb* contexto_ejecucion, Instruccion instruccion, int fd, int fd_memoria);
int server_escuchar(int server_socket_dispatch, int server_socket_interupt);
#endif