#ifndef MAIN_H_
#define MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <utils/utils.h>
#include <sockets/sockets.h>
#include <comunicacion.h>

int  conexion_cpu_memoria;
char* ip_memoria;
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;

t_log* logger_cpu;
t_config* config;

#endif 