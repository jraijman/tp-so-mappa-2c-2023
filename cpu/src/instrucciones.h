#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_


#include "main.h"
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "../../utils/src/protocolo/protocolo.h"

int tamPaginaGlobal;
int traducir(int direccionLogica, int fd_memoria, int pid, int fd_dispatch);
uint32_t* obtener_registro(pcb* contexto, char* nombre_registro);
void setInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger);
void sumInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger);
void subInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger);
void jnzInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger);
void sleepInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger, int fd_cpu_dispatch);
void waitInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger, int fd_cpu_dispatch);
void signalInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger, int fd_cpu_dispatch);
void exitInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger , int fd_cpu_dispatch);
void movInInstruccion(pcb* contexto, Instruccion instruccion,int direccionFisica, t_log* logger);
void movOutInstruccion(pcb* contexto, Instruccion instruccion,int direccionFisica,int fd_memoria, t_log* logger);
void fOpenInstruccion(pcb* contexto, Instruccion instruccion, int fd_cpu_dispatch, t_log* logger);
void fCloseInstruccion(pcb* contexto, Instruccion instruccion, int fd_cpu_dispatch, t_log* logger);
void fSeekInstruccion(pcb* contexto, Instruccion instruccion,int direccionFisica, int fd_cpu_dispatch, t_log* logger);
void fReadInstruccion(pcb* contexto, Instruccion instruccion,int direccionFisica, int fd_cpu_dispatch,int fd_memoria, t_log* logger);
void fWriteInstruccion(pcb* contexto, Instruccion instruccion,int direccionFisica, int fd_cpu_dispatch,int fd_memoria, t_log* logger);
void fTruncateInstruccion(pcb* contexto, Instruccion instruccion, int fd_cpu_dispatch, t_log* logger);

#endif 
