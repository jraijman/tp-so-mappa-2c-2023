#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>


t_config* iniciar_config(char* ruta);
t_log* iniciar_logger(char* file, char *process_name);
void terminar_programa(t_log* logger, t_config* config);
#endif 